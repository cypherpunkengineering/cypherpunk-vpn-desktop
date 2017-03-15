'use strict';

var gulp = require('gulp');
var runSequence = require('run-sequence');
var gutil = require('gulp-util');
var concat = require('gulp-concat');
var rename = require('gulp-rename');
var less = require('gulp-less');
var filter = require('gulp-filter');
var clean = require('gulp-clean');
var shell = require('gulp-shell');
var sourcemaps = require('gulp-sourcemaps');
var newer = require('gulp-newer');
var jsonTransform = require('gulp-json-transform');
var babel = require('gulp-babel');
var install = require('gulp-install');
var download = require('gulp-download-stream');
var replace = require('gulp-replace');
var through = require('through');
var { fork, spawn, exec } = require('child_process');
var fs = require('fs');
var es = require('event-stream');
var stream = require('stream');
var webpack = require('webpack');

var packageJson = require('./package.json');
var webpackConfig = require('./webpack.config');


gulp.task('default', ['build']);

gulp.task('build', ['build-app']);

gulp.task('clean', ['clean-app', 'clean-semantic', 'clean-web-libraries', 'clean-web-fonts']);

gulp.task('watch', ['build'], function() {
  runSequence([
    'watch-semantic',
    'watch-main',
    'watch-web',
    'watch-app-nodemodules',
  ]);
});

gulp.task('version', function() {
  // The package.json version number has been changed; propagate to
  // all other locations that don't automatically pick it up.
  var version = packageJson.version;
  function replaceVersion(filename, ...operations) {
    return operations.reduce((a, b) => a.pipe(b), gulp.src(filename, { base: './' })).pipe(gulp.dest('.'));
  }
  return es.merge(
    replaceVersion('../daemon/version.h', replace(/^(#define VERSION ).*/m, `$1"${version}"`)),
    replaceVersion('../build/win/setup.iss', replace(/^(#define MyAppVersion ).*/m, `$1"${version}"`), replace(/^(#define MyAppNumericVersion ).*/m, `$1"${version.replace(/[-+].*/,'')}"`), replace(/^(#define MyInstallerSuffix ).*/m, `$1"-${version.replace('+','-')}"`))
  );
});


/***************************************************************************
 * APP: Perform all tasks necessary to produce the app/ directory.
 */
gulp.task('build-app', ['build-app-nodemodules', 'build-main', 'build-web' ]);
gulp.task('clean-app', function() {
  return gulp.src('app', { read: false })
    .pipe(clean());
});

/***************************************************************************
 * APP NODE MODULES: Install required node modules in the app/ directory.
 */
gulp.task('build-app-nodemodules', ['build-app-packagejson'], function() {
  return gulp.src('app/package.json')
    .pipe(install({ production: true }));
});
gulp.task('watch-app-nodemodules', function() {
  gulp.watch('app/package.json', ['build-app-nodemodules']);
});

/***************************************************************************
 * APP PACKAGE.JSON: Automatically create the app-specific package.json in
 * the app/ directory stripping out unwanted parts from the main file.  
 */
gulp.task('build-app-packagejson', function() {
  return gulp.src('package.json')
    .pipe(jsonTransform(function(data, file) {
      var result = {};
      Object.assign(result, data);
      ['scripts', 'engines', 'devDependencies'].forEach(e => delete result[e]);
      if (result['platformDependencies'])
        result['dependencies'] = Object.assign({}, result['dependencies'], result['platformDependencies'][process.platform]);
      delete result['platformDependencies'];
      return result;
    }))
    .pipe(gulp.dest('app'));
});

/***************************************************************************
 * MAIN: Perform all tasks required to produce the scripts and assets that
 * will run in the Electron main process (in the app/ directory).  
 */
gulp.task('build-main', ['build-main-assets'], function() {
  var p = gulp.src(['src/**/*.js', '!src/web', '!src/web/**/*', '!src/assets', '!src/assets/**/*'])
    .pipe(newer('app'));
  if (process.env.NODE_ENV !== 'production')
    p = p.pipe(sourcemaps.init());
  p = p.pipe(babel());
  if (process.env.NODE_ENV !== 'production')
    p = p.pipe(sourcemaps.write('.'));
  return p.pipe(gulp.dest('app'));
});
gulp.task('build-main-assets', function() {
  return gulp.src('src/assets/**/*')
    .pipe(newer('app/assets'))
    .pipe(gulp.dest('app/assets'));
})
gulp.task('watch-main', function() {
  gulp.watch(['src/**/*.js', '!src/web', '!src/web/**/*'], ['build-main']);
});

/***************************************************************************
 * WEB: Perform all tasks necessary to produce the app/web/ directory.
 */
gulp.task('build-web', ['build-webpack']);
//gulp.task('build-web', ['build-webpack'], function() {
//  return gulp.src('src/web/lib/**/*')
//    .pipe(gulp.dest('app/web/lib'));
//});
gulp.task('watch-web', ['watch-webpack']);

/***************************************************************************
 * WEBPACK: Bundle up all sources from src/web/ and referenced node modules
 * and put the resulting output files in app/web/.
 */
gulp.task('build-webpack', ['build-semantic', 'build-web-libraries', 'build-web-fonts'], function() {
  return new Promise((resolve, reject) => {
    let logger = webpackLogger(resolve, reject);
    try {
      webpack(webpackConfig).run(logger);
    } catch (e) {
      logger(e);
    }
  });
});
gulp.task('watch-webpack', function() {
  webpack(webpackConfig).watch({}, webpackLogger(() => {}, () => {}));
});

/***************************************************************************
 * SEMANTIC: Compile the Semantic UI sources from semantic/ and put the
 * output in src/web/semantic/ (where it can be picked up by webpack).
 */
gulp.task('build-semantic', function(callback) {
  gulp.src('semantic/**/*', { read: false })
    .pipe(newer('src/web/semantic/index.js')) // this is the last generated file
    .pipe(ifEmpty(function() {
      gutil.log("Semantic already up to date");
      callback();
    }, function() {
      spawn('"../node_modules/.bin/gulp"', [ 'build' ], { cwd: 'semantic', shell: true, stdio: 'inherit' })
        .on('exit', makeSemanticWebpackShims(callback));
    }));
});
gulp.task('clean-semantic', function() {
  return gulp.src('src/web/semantic')
    .pipe(clean());
});
gulp.task('watch-semantic', function(callback) {
  spawn('"../node_modules/.bin/gulp"', [ 'watch' ], { cwd: 'semantic', shell: true, stdio: 'inherit' })
});

/***************************************************************************
 * WEB LIBRARIES: Download any javascript libraries mentioned in a special
 * 'webLibraries' section of package.json and put them in src/web/lib/.
 */
gulp.task('build-web-libraries', function() {
  var requests = [];
  for (var file in (packageJson.webLibraries || {})) {
    if (!fs.existsSync('src/web/lib/' + file + '.js')) {
      requests.push({ file: file + '.js', url: packageJson.webLibraries[file].replace(/\.min\.js$/, '.js') });
    }
    if (!fs.existsSync('src/web/lib/' + file + '.min.js')) {
      requests.push({ file: file + '.min.js', url: packageJson.webLibraries[file] });
    }
  }
  return download(requests)
    .pipe(gulp.dest('src/web/lib'));
});
gulp.task('clean-web-libraries', function() {
  return gulp.src(Object.keys(packageJson.webLibraries || {}).map(m => 'src/web/lib/' + m + '?(.min).js'), { read: false })
    .pipe(clean());
});

/***************************************************************************
 * WEB FONTS: Download any webfonts mentioned in a special 'webFonts'
 * section of package.json and put them in src/web/assets/fonts/.
 */
gulp.task('build-web-fonts', function(callback) {
  var fonts = [];
  for (var name in (packageJson.webFonts || {})) {
    if (!fs.existsSync('src/web/assets/fonts/' + name + '.css')) {
      fonts.push({ file: name + '.css', url: packageJson.webFonts[name] });
    }
  }
  var urls = [];
  download(fonts, { headers: { 'User-Agent': 'Mozilla/5.0 AppleWebKit/537.36 Chrome/53.0.2785.116 Safari/537.36' }})
    .pipe(replace(/url\(([^)]*)\)/g, function(s, url) {
      var file = url.split('/').pop();
      gutil.log("Replacing " + url + " with " + file);
      urls.push({ file: file, url: url });
      return "url(" + file + ")";
    }))
    .pipe(gulp.dest('src/web/assets/fonts'))
    .on('end', function() {
      download(urls)
        .pipe(gulp.dest('src/web/assets/fonts'))
        .on('end', callback);
    });
});
gulp.task('clean-web-fonts', function() {
  return gulp.src('src/web/assets/fonts/*', { read: false })
    .pipe(clean());
});



/**************************************************************************/

// Helper function: call one callback if stream is empty (no files), and another otherwise.
function ifEmpty(isEmpty, isNotEmpty) {
  var empty = true;
  return new stream.Transform({
    objectMode: true,
    transform: function(chunk, encoding, callback) { if (empty) { isNotEmpty(); empty = false; } callback(null, chunk); },
    flush: function(callback) { if (empty) { isEmpty(); } callback(); }
  })
}

// Helper function: print out the results from webpack, or throw an error on failure.
function webpackLogger(resolve, reject) {
  return function(err, stats) {
    if (err) {
      console.dir(err);
      reject(new gutil.PluginError('webpack', { message: 'Fatal error: ' + err.toString() }));
    } else {
      var log = stats.toString({
        chunks: false,
        errors: true,
        //errorDetails: true,
        assets: false,
        children: false,
        hash: false,
        timings: false,
        version: false,
        cached: false,
        colors: true
      });
      if (stats.hasErrors()) {
        reject(new gutil.PluginError('webpack', { message: "Webpack compile error(s):\n" + log}));
      } else {
        gutil.log("Webpack completed successfully:\n" + log);
        resolve();
      }
    }
  };
}

// Helper function: make webpack shims for each js/css file in Semantic UI (so we can just require the component name)
function makeSemanticWebpackShims(callback) {
  return function() {
    gulp.src(['src/web/semantic/components/*.min.js', 'src/web/semantic/components/*.min.css'], { read: false })
      .pipe((function(){
        var files = {};
        function onFile(file) {
          var f = file.relative.replace('\\', '/');
          var m = f.replace(/\.min\.(js|css)$/, '');
          var r = f.replace(/^([^/]*\/)*/, '');
          (files[m] || (files[m] = [])).push(r);
        }
        function onEnd() {
          for (var e in files) {
            this.emit('data', new gutil.File({
              cwd: '.', base: 'src/web/semantic', path: 'src/web/semantic/components/' + e + '.webpack', contents: new Buffer(files[e].sort().map(f => "require('./" + f + "');\n").join(''))
            }));
          }
          this.emit('data', new gutil.File({
            cwd: '.', base: 'src/web/semantic', path: 'src/web/semantic/index.js', contents: new Buffer(Object.keys(files).map(f => "require('./components/" + f + ".webpack');\n").join(''))
          }));
          this.emit('end');
        }
        return through(onFile, onEnd);
      })())
      .pipe(gulp.dest('src/web/semantic'))
      .on('end', callback);
  };
}

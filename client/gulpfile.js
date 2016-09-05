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
var through = require('through');
var File = require('vinyl');
var { fork, spawn, exec } = require('child_process');
var stream = require('stream');
var webpack = require('webpack');

var packageJson = require('./package.json');
var webpackConfig = require('./webpack.config');


gulp.task('default', ['build']);

gulp.task('build', ['build-app']);

gulp.task('clean', ['clean-app', 'clean-semantic', 'clean-web-libraries']);

gulp.task('watch', ['build'], function() {
  runSequence([
    'watch-semantic',
    'watch-main',
    'watch-web',
    'watch-app-nodemodules',
  ]);
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
      return result;
    }))
    .pipe(gulp.dest('app'));
});

/***************************************************************************
 * MAIN: Perform all tasks required to produce the scripts and assets that
 * will run in the Electron main process (in the app/ directory).  
 */
gulp.task('build-main', ['build-main-assets'], function() {
  return gulp.src(['src/**/*.js', '!src/web', '!src/web/**/*', '!src/assets', '!src/assets/**/*'])
    .pipe(newer('app'))
    .pipe(sourcemaps.init())
    .pipe(babel())
    .pipe(sourcemaps.write('.'))
    .pipe(gulp.dest('app'));
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
gulp.task('build-webpack', ['build-semantic', 'build-web-libraries'], function(callback) {
  webpack(webpackConfig).run(webpackLogger(callback));
});
gulp.task('watch-webpack', function() {
  webpack(webpackConfig).watch({}, webpackLogger(function(){}));
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
    requests.push({ file: file + '.min.js', url: packageJson.webLibraries[file] });
  }
  return download(requests)
    .pipe(gulp.dest('src/web/lib'));
});
gulp.task('clean-web-libraries', function() {
  return gulp.src(Object.keys(packageJson.webLibraries || {}).map(m => 'src/web/lib/' + m + '.min.js'), { read: false })
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
function webpackLogger(callback) {
  return function(err, stats) {
    if (err) throw new gutil.PluginError('webpack', err);
    gutil.log("Webpack completed successfully:\n" +
      stats.toString({
        hash: false,
        version: false,
        cached: false,
        colors: true
      }));
    callback();
  };
}

// Helper function: make webpack shims for each js/css file in Semantic UI (so we can just require the component name)
function makeSemanticWebpackShims(callback) {
  return function() {
    gulp.src(['src/web/semantic/**/*.min.js', 'src/web/semantic/**/*.min.css'], { read: false })
      .pipe((function(){
        var files = {};
        function onFile(file) {
          var f = file.relative.replace('\\', '/');
          var m = f.replace(/\.min\.(js|css)$/, '');
          var r = f.replace(/^([^/]*\/)*/, '');
          (files[m] || (files[m] = [])).push('./' + r);
        }
        function onEnd() {
          for (var e in files) {
            var n = e.endsWith('semantic') ? 'index.js' : e + '.webpack.js';
            this.emit('data', new File({
              cwd: '.', base: 'src/web/semantic', path: 'src/web/semantic/' + n, contents: new Buffer(files[e].sort().map(f => "require('" + f + "');\n").join(''))
            }));
          }
          this.emit('end');
        }
        return through(onFile, onEnd);
      })())
      .pipe(gulp.dest('src/web/semantic'))
      .on('end', callback);
  };
}

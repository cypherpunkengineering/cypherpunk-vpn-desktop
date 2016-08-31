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
var { fork, spawn, exec } = require('child_process');
var stream = require('stream');
var webpack = require('webpack');

var packageJson = require('./package.json');
var webpackConfig = require('./webpack.config');


gulp.task('default', ['build']);

gulp.task('build', ['clean'], function(callback) {
  runSequence('build-semantic', 'build-webpack', 'build-main', callback);
});

gulp.task('clean', ['clean-web-libraries'], function() {
  return gulp.src('app', { read: false })
    .pipe(clean());
});

// Helper object: call one callback if stream is empty (no files), and another otherwise
function ifEmpty(isEmpty, isNotEmpty) {
  var empty = true;
  return new stream.Transform({
    objectMode: true,
    transform: function(chunk, encoding, callback) { if (empty) { isNotEmpty(); empty = false; } callback(null, chunk); },
    flush: function(callback) { if (empty) { isEmpty(); } callback(); }
  })
}

gulp.task('build-semantic', function(callback) {
  gulp.src('semantic/**/*', { read: false })
    .pipe(newer('src/web/semantic/semantic.css')) // this is the last generated file
    .pipe(ifEmpty(function() {
      gutil.log("Semantic already up to date");
      callback();
    }, function() {
      spawn('"../node_modules/.bin/gulp"', [ 'build' ], { cwd: 'semantic', shell: true, stdio: 'inherit' }).on('exit', callback);
    }));
});
gulp.task('clean-semantic', function() {
  return gulp.src('src/web/semantic')
    .pipe(clean());
  //spawn('"../node_modules/.bin/gulp"', [ 'clean', '--force' ], { cwd: 'semantic', shell: true, stdio: 'inherit' }).on('exit', callback);  
});

gulp.task('build-webpack', ['build-semantic'/*,'build-web-libraries'*/], function(callback) {
  // TODO: Only build if needed?
  //gulp.src('app/js/app.js')
  //  .pipe(webpack(webpackConfig))
  //  .pipe(gulp.dest('build'));
  webpack(webpackConfig).run(function(err, stats) {
    if (err) throw new gutil.PluginError('webpack', err);
    gutil.log("Webpack completed successfully:\n" +
      stats.toString({
        hash: false,
        version: false,
        cached: false,
        colors: true
      }));
    callback();
  });
});

gulp.task('build-web-libraries', function() {
  var requests = [];
  for (var file in (packageJson.webLibraries || {})) {
    requests.push({ file: file + '.js', url: packageJson.webLibraries[file] });
  }
  return download(requests)
    .pipe(gulp.dest('src/web/lib'));
});
gulp.task('clean-web-libraries', function() {
  return gulp.src(Object.keys(packageJson.webLibraries || {}).map(m => 'src/web/lib/' + m + '.js'), { read: false })
    .pipe(clean());
});

gulp.task('build-app', ['build-app-nodemodules', 'build-main', 'build-web' ]);

gulp.task('build-web', ['build-webpack'], function() {
  return gulp.src('src/web/lib/**/*')
    .pipe(gulp.dest('app/web/lib'));
});

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

gulp.task('build-app-nodemodules', ['build-app-packagejson'], function() {
  return gulp.src('app/package.json')
    .pipe(install({ production: true }));
});

gulp.task('build-main', ['build-main-assets'], function() {
  return gulp.src(['src/**/*.js', '!src/web', '!src/web/**/*', '!src/assets', '!src/assets/**/*'])
    .pipe(sourcemaps.init())
    .pipe(babel())
    .pipe(sourcemaps.write('.'))
    .pipe(gulp.dest('app'));
});

gulp.task('build-main-assets', function() {
  return gulp.src('src/assets/**/*')
    .pipe(gulp.dest('app/assets'));
})


// TODO: Support watch


if (false) {


gulp.task('watch', function() {
  runSequence(
    'build',
    [
      'watch-src',
      'watch-vendor'
    ]
  );
});

gulp.task('build', function(callback) {
  runSequence(
    'clean:dist',
    'build-src',
    'build-vendor',
    callback
  );
});

gulp.task('build-src', [
  'build-src:js',
  'build-src:css',
  'build-src:html'
]);

gulp.task('build-vendor', [
  'build-vendor:js',
  'build-vendor:css',
  'build-vendor:semantic-ui'
]);

gulp.task('watch-src', [
  'watch-src:js',
  'watch-src:css',
  'watch-src:html'
]);

gulp.task('watch-vendor', function() {
  gulp.watch('bower.json' ['build-vendor']);
});

gulp.task('watch-src:js', function() {
  webpack(webpackConfig).watch({}, webpackLogger());
});

gulp.task('watch-src:css', function() {
  gulp.watch('./src/less/**/*', ['build-src:css']);
});

gulp.task('watch-src:html', function() {
  gulp.watch('./src/html/**/*', ['build-src:html']);
});

gulp.task('build-src:js', function(callback) {
  webpack(webpackConfig).run(webpackLogger(callback));
});

gulp.task('build-src:css', function() {
  return gulp.src('src/less/main.less')
    .pipe(sourcemaps.init())
    .pipe(less())
    .pipe(rename('main.css'))
    .pipe(sourcemaps.write('./'))
    .pipe(gulp.dest('dist'));
});

gulp.task('build-src:html', function() {
  return gulp.src('src/html/main.html')
    .pipe(gulp.dest('dist'));
});

gulp.task('build-vendor:js', function() {
  return gulp.src(mainBowerFiles())
    .pipe(filter(['**/*.js']))
    .pipe(sourcemaps.init())
    .pipe(concat('vendor.js'))
    .pipe(sourcemaps.write('./'))
    .pipe(gulp.dest('dist'));
});

gulp.task('build-vendor:css', function() {
  return gulp.src(mainBowerFiles())
    .pipe(filter(['**/*.css']))
    .pipe(sourcemaps.init())
    .pipe(concat('vendor.css'))
    .pipe(sourcemaps.write('./'))
    .pipe(gulp.dest('dist'));
});

gulp.task('build-vendor:semantic-ui', function() {
  return gulp.src('bower_components/semantic-ui/dist/**/*')
    .pipe(gulp.dest('dist/semantic-ui'));
});

gulp.task('clean:dist', function() {
  return gulp.src('dist')
    .pipe(clean());
});

gulp.task('build-electron', function(callback) {
  runSequence(
    'build',
    'build-electron:clean',
    'build-electron:src',
    callback
  );
});

gulp.task('build-electron:clean', function() {
  return gulp.src('Electron.app')
    .pipe(clean());
});

gulp.task('build-electron:src', shell.task([
  'cp -a node_modules/electron-prebuilt/dist/Electron.app ./',
  'mkdir Electron.app/Contents/Resources/app',
  'cp main.js Electron.app/Contents/Resources/app/',
  'cp package.json Electron.app/Contents/Resources/app/',
  'cp -a dist Electron.app/Contents/Resources/app/'
]));

}

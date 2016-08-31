var webpack = require('webpack');
var nodeExternals = require('webpack-node-externals');
var webpackTargetElectronRenderer = require('webpack-target-electron-renderer');
//const path = require('path');

var options = {
  target: 'node',
  entry: {
    app: [ './app/js/app.js' ],
  },
  externals: [nodeExternals()],
  output: {
    path: __dirname + '/app/build/',
    filename: 'bundle.js'
  },
  resolve: {
     modulesDirectories: ['node_modules'],
     extensions: ['', '.js', '.jsx', '.css', '.scss']
   },
  devServer: {
    contentBase: '.',
    publicPath: 'http://localhost:8080/built/'
  },
  devtool: "source-map",
  module: {
    loaders: [
      { test: /\.jsx?$/, exclude: /node_modules/, loader: 'babel', query: { presets: ['es2015', 'react'], minified: true, comments: false } },
      { test: /\.css$/, loader: 'style!css' },
      { test: /\.less$/, loader: 'style!css!less'},
      { test: /\.scss$/, loader: 'style!css?sourceMap!sass?sourceMap' },
      //{ test: /\.(ttf|otf|woff2?)(\?v=\d+\.\d+\.\d+)?$/, loader: 'file?mimetype=application/octet-stream'}
    ]
  },
  plugins: [
    new webpack.HotModuleReplacementPlugin(),
    new webpack.IgnorePlugin(new RegExp("^(fs|ipc)$")),
    new webpack.optimize.UglifyJsPlugin({minimize: true})
  ]
}
options.target = webpackTargetElectronRenderer(options);
module.exports = options;

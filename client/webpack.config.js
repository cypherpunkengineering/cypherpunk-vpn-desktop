var webpack = require('webpack');
var nodeExternals = require('webpack-node-externals');
var webpackTargetElectronRenderer = require('webpack-target-electron-renderer');
const autoprefixer = require('autoprefixer');
const ExtractTextPlugin = require('extract-text-webpack-plugin');
const path = require('path');

const pkg = require('./package.json');

var options = {
  target: 'node',
  entry: {
    app: [ './app/js/app.js' ],
  },
  externals: [Object.keys(pkg.dependencies)],
  output: {
    path: __dirname + '/app/build/',
    filename: 'bundle.js'
  },
  // doesnt seem to help
  resolve: {
    modulesDirectories: [
      'node_modules',
      path.resolve(__dirname, './node_modules')
    ]
  },
  devServer: {
    contentBase: '.',
    publicPath: 'http://localhost:8080/built/'
  },
  devtool: "source-map",
  module: {
    loaders: [
      { test: /\.jsx?$/, exclude: /node_modules/, loader: 'babel', query: { presets: ['es2015', 'react'], minified: true, comments: false } },
      //{ test: /\.css$/, loader: 'style!css' },
      //{ test: /\.less$/, loader: 'style!css!less'},
      {
        test: /(\.scss|\.css)$/,
          loader: ExtractTextPlugin.extract(
            'style',
            'css?sourceMap&modules&importLoaders=1&localIdentName=[name]__[local]___[hash:base64:5]!postcss!sass'
          )
      },
    //  { test: /\.scss$/, loader: 'style!css?sourceMap!sass?sourceMap' },
      //{ test: /\.(ttf|otf|woff2?)(\?v=\d+\.\d+\.\d+)?$/, loader: 'file?mimetype=application/octet-stream'}
    ]
  },
  postcss: [autoprefixer],
  sassLoader: {
    //data: '@import "theme/_config.scss";',
    includePaths: [path.resolve(__dirname, './src/app')]
  },
  plugins: [
    new ExtractTextPlugin('bundle.css', { allChunks: true }),
    new webpack.HotModuleReplacementPlugin(),
    new webpack.IgnorePlugin(new RegExp("^(fs|ipc)$")),
    new webpack.optimize.UglifyJsPlugin({minimize: true})
  ]
}
options.target = webpackTargetElectronRenderer(options);
module.exports = options;

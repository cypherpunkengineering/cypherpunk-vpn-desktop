const webpack = require('webpack');
const webpackTargetElectronRenderer = require('webpack-target-electron-renderer');
const ExtractTextPlugin = require('extract-text-webpack-plugin');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const path = require('path');

const pkg = require('./package.json');

const ENV = process.env.NODE_ENV || 'development';
const development = ENV === 'development';
const production = !development;
const extractCss = true;
const useWebLibraries = production;
const libExtension = production ? '.min.js' : '.js';

const devMap = production ? '' : '?sourceMap';
function cssLoader(a, b) { return extractCss ? ExtractTextPlugin.extract(a, b) : (a + '!' + b); }


var options = {
  target: 'node',
  context: path.resolve(__dirname, 'src/web'),
  entry: {
    'app': [ /*'webpack-dev-server',*/ './index.js' ],
  },
  output: {
    path: path.resolve(__dirname, 'app/web'),
    filename: '[name].js',
    libraryTarget: 'commonjs2'
  },
  resolve: {
    modulesDirectories: production ? [] : [
      path.resolve(__dirname, 'node_modules'),
      'node_modules',
    ],
    alias: {
      'semantic': path.resolve(__dirname, 'src/web/semantic'),
    },
    extensions: [ '', '.webpack', '.webpack.js', libExtension, '.min.js', '.js', '.jsx' ],
  },
  externals: [
    Object.keys(pkg.dependencies || {}), // exclude packaged node modules
  ],
  module: {
    loaders: [
      { test: /./, include: path.resolve(__dirname, 'src/assets'), loader: 'file-loader?emitFile=false&name=../[path][name].[ext]&context=src' },
      { test: /\.jsx?$/, exclude: /(node_modules|[\/]~[\/]|semantic[\/]|webpack\-dev\-server|socket\.io\-client|\.min\.js$)/, loader: 'babel' },
      { test: /\.css$/, loader: cssLoader('style' + devMap, 'css' + devMap) },
      { test: /\.less$/, loader: cssLoader('style' + devMap, 'css' + devMap + '!less' + devMap) },
      { test: /\.scss$/, loader: cssLoader('style' + devMap, 'css' + devMap + '!sass' + devMap) },
      //{ test: /\.(css|less)$/, loader: 'style!css!less' /*ExtractTextPlugin.extract('style?sourceMap', 'css?sourceMap!postcss!less?sourceMap')*/ },
      { test: /\.(ttf|otf|eot|woff2?)(\?v=\d+\.\d+\.\d+)?$/, loader: 'file' },
      { test: /\.(png|gif|svg)$/, loader: 'file' },
    ]
  },
  //postcss: function() { return autoprefixer({ browsers: [ 'Chrome >= 52' ] }); },
  plugins: [
    //new webpack.NoErrorsPlugin(),
    new ExtractTextPlugin('[name].css', { allChunks: true, disable: !extractCss }),
    new webpack.optimize.DedupePlugin(),
    new webpack.optimize.OccurenceOrderPlugin(),
    new webpack.DefinePlugin({ 'process.env': { 'NODE_ENV': JSON.stringify(ENV) } }),
    new webpack.IgnorePlugin(new RegExp("^(fs|ipc)$")), // what is this?
    new webpack.ProvidePlugin({
      $: "jquery",
      jQuery: "jquery",
      "global.jQuery": "jquery"
    }),
    new HtmlWebpackPlugin({ title: 'Cypherpunk Privacy', filename: 'index.html' }),
    new webpack.HotModuleReplacementPlugin(),
  ],
  node: { console: false, global: false, process: false, Buffer: false, __filename: false, __dirname: false, setImmediate: false },
  stats: { colors: true },
  devtool: ENV === 'development' ? 'inline-source-map' : 'source-map',
  devServer: {
    port: process.env.PORT || 8080,
    contentBase: './build', // TODO: correct?
    inline: true,
    hot: true,
    historyApiFallback: true
  }
}

if (useWebLibraries) {
  options.resolve.modulesDirectories.unshift(path.resolve(__dirname, 'src/web/lib'));
  Object.assign(options.resolve.alias, {
    'react/addons': 'react',
    'react/lib': 'react',
    'react/dist': 'react',
    'react-addons-css-transition-group': 'react',
    'react-addons-linked-state-mixin': 'react',
    'react-addons-clone-with-props': 'react',
    'react-addons-create-fragment': 'react',
    'react-addons-update': 'react',
    'react-addons-pure-render-mixin': 'react',
    'react-addons-shallow-compare': 'react',
  });
}

options.target = webpackTargetElectronRenderer(options);

module.exports = options;

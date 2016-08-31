const webpack = require('webpack');
const webpackTargetElectronRenderer = require('webpack-target-electron-renderer');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const path = require('path');

const pkg = require('./package.json');

const ENV = process.env.NODE_ENV || 'development';

var options = {
  //target: 'electron',
  context: path.resolve(__dirname, 'src/web'),
  entry: {
    'app': [ './index.jsx' ],
  },
  output: {
    path: path.resolve(__dirname, 'app/web'),
    filename: '[name].bundle.js',
    libraryTarget: 'commonjs2'
  },
  resolve: {
		modulesDirectories: [
      path.resolve(__dirname, 'src/web/lib'),
      path.resolve(__dirname, 'node_modules'),
			'node_modules'
		],
    //extensions: [ '', '.min.js', '.js' ],
    //alias: Object.keys(pkg.webLibraries).map(f => ({ [f]: '/lib/' + f + '.js' }))
	},
  //resolveLoader: {
  //  root: path.resolve(__dirname, 'node_modules')
  //},
  externals: [
    { 'remote': 'commonjs remote' },
    '../rpc.js', // FIXME: exclude all files outside web/ directory
    Object.keys(pkg.dependencies || {})
	],
  //devServer: {
  //  contentBase: '.',
  //  publicPath: 'http://localhost:8080/built/'
  //},
  //devtool: "source-map",
  module: {
    loaders: [
      { test: /\.jsx?$/, exclude: /(node_modules|\/~\/|webpack\-dev\-server|socket\.io\-client)/, loader: 'babel' },
      { test: /\.(css|less)$/, loader: 'style!css!less' /*ExtractTextPlugin.extract('style?sourceMap', 'css?sourceMap!postcss!less?sourceMap')*/ },
      { test: /\.(ttf|otf|eot|woff2?)(\?v=\d+\.\d+\.\d+)?$/, loader: 'file' },
      { test: /\.(png|gif|svg)$/, loader: 'file' },
    ]
  },
  //postcss: function() { return autoprefixer({ browsers: [ 'Chrome >= 52' ] }); },
  plugins: [
    //new webpack.NoErrorsPlugin(),
    //new ExtractTextPlugin('style.css', { allChunks: true, disable: ENV !== 'production' }),
    new webpack.optimize.DedupePlugin(),
    new webpack.optimize.OccurenceOrderPlugin(),
    new webpack.DefinePlugin({ 'process.env': { 'NODE_ENV': JSON.stringify(ENV) } }),
    new webpack.IgnorePlugin(new RegExp("^(fs|ipc)$")),
    new webpack.ProvidePlugin({
      $: "jquery",
      jQuery: "jquery",
      "global.jQuery": "jquery"
    }),
    new HtmlWebpackPlugin({ title: 'Cypherpunk VPN', filename: 'index.html', inject: 'head', minify: { collapseWhitespace: true } }),
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

options.target = webpackTargetElectronRenderer(options);

module.exports = options;

var webpack = require('webpack');

module.exports = {
  target: 'node',
  entry: {
  app: ['webpack/hot/dev-server', './app/js/app.js'],
},
output: {
  path: './app/build/',
  filename: 'bundle.js'
},
devServer: {
  contentBase: '.',
  publicPath: 'http://localhost:8080/built/'
},


module: {
 loaders: [
   { test: /.jsx?$/, exclude: /node_modules/, loader: 'babel-loader',
      query: { presets:['es2015', 'react'] }
    },
   { test: /\.css$/, loader: 'style-loader!css-loader' },
   { test: /\.less$/, loader: 'style-loader!css-loader!less-loader'},
   { test: /\.ttf(\?v=\d+\.\d+\.\d+)?$/, loader: 'file-loader?mimetype=application/octet-stream'}
 ]
},
 plugins: [
   new webpack.HotModuleReplacementPlugin(),
   new webpack.IgnorePlugin(new RegExp("^(fs|ipc)$"))
 ]
}

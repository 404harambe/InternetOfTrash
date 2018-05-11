const { resolve } = require('path');
const webpack = require('webpack');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const CopyWebpackPlugin = require('copy-webpack-plugin');
const ExtractTextPlugin = require('extract-text-webpack-plugin');

// Config file path is passed with an env variable
let configFilePath = process.env['CONFIG_FILE'];
if (!configFilePath) {
    throw new Error('Missing config file path. Please, provide the path with the environment variable CONFIG_FILE.');
}
configFilePath = resolve(process.cwd(), configFilePath);

const config = {
  devtool: 'cheap-module-source-map',

  entry: [
    './main.js',
    './assets/scss/main.scss',
  ],

  context: resolve(__dirname, 'app'),

  output: {
    filename: 'bundle.js',
    path: resolve(__dirname, 'dist'),
    publicPath: '',
  },

  plugins: [
    new webpack.optimize.ModuleConcatenationPlugin(),
    new HtmlWebpackPlugin({
      template: `${__dirname}/build/index.html`,
      filename: 'index.html',
      inject: false,
    }),
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.LoaderOptionsPlugin({
      minimize: true,
      debug: false,
    }),
    new webpack.optimize.UglifyJsPlugin({
      beautify: false
    }),
    new webpack.DefinePlugin({ 'process.env': { NODE_ENV: JSON.stringify('production') } }),
    new ExtractTextPlugin({ filename: './styles/style.css', disable: false, allChunks: true }),
  ],

  resolve: {
    extensions: ['.js', '.jsx'],
    alias: {
      'config': configFilePath
    },
  },

  resolveLoader: {
    modules: ['node_modules', resolve(__dirname, 'build')]
  },

  module: {
    loaders: [
      {
        test: /config\.ini$/,
        loader: 'config-loader'
      },
      {
        test: /\.jsx?$/,
        exclude: /node_modules/,
        loader: 'babel-loader',
      },
      {
        test: /\.s?css$/,
        use: ExtractTextPlugin.extract({
          fallback: 'style-loader',
          use: [
            'css-loader',
            { loader: 'sass-loader', query: { sourceMap: false } },
          ],
          publicPath: '../'
        }),
      },
      {
        test: /\.(png|jpg|gif)$/,
        use: [
          {
            loader: 'url-loader',
            options: {
              limit: 8192,
              mimetype: 'image/png',
              name: 'images/[name].[ext]',
            }
          }
        ],
      },
      {
        test: /\.eot(\?v=\d+.\d+.\d+)?$/,
        use: [
          {
            loader: 'file-loader',
            options: {
              name: 'fonts/[name].[ext]'
            }
          }
        ],
      },
      {
        test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        use: [
          {
            loader: 'url-loader',
            options: {
              limit: 8192,
              mimetype: 'application/font-woff',
              name: 'fonts/[name].[ext]',
            }
          }
        ],
      },
      {
        test: /\.[ot]tf(\?v=\d+.\d+.\d+)?$/,
        use: [
          {
            loader: 'url-loader',
            options: {
              limit: 8192,
              mimetype: 'application/octet-stream',
              name: 'fonts/[name].[ext]',
            }
          }
        ],
      },
      {
        test: /\.svg(\?v=\d+\.\d+\.\d+)?$/,
        use: [
          {
            loader: 'url-loader',
            options: {
              limit: 8192,
              mimetype: 'image/svg+xml',
              name: 'images/[name].[ext]',
            }
          }
        ],
      },
    ]
  },
};

module.exports = config;

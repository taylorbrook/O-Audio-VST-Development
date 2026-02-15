const path = require('path');

module.exports = {
  entry: './src/app.js',
  output: {
    filename: 'app.bundle.js',
    path: path.resolve(__dirname, 'public/js'),
  },
  target: 'web',
  resolve: {
    extensions: ['.js'],
  },
};

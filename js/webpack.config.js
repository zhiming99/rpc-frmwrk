// webpack.config.js
let path = require('path')

const webpack=require('webpack')
module.exports = {
    mode:'development',
    entry: './src/index.js',
    output:{
        path:path.resolve(__dirname,'dist'),
        filename:'rpc-frmwrk.js',
        globalObject: 'this',
        library: {
            name: 'rpcbase',
            type: 'umd',
        }
    },
    plugins: [
        new webpack.ProvidePlugin({
        process: 'process/browser',
        }),
        new webpack.ProvidePlugin({
        Buffer: ['buffer', 'Buffer'],
        }),
    ],
    resolve: {
        // Tells Webpack: "Stop evaluating the absolute global path.
        // Treat this folder as a local directory right here in the project."
        symlinks: false,
        // Explicitly verify Webpack checks your linked folder name
        modules: ['node_modules']
    },
    module: {
        rules: [
            {
            test: /\.js$/,
            // Because symlinks is false, Webpack shields everything inside
            // the local node_modules folder—preventing the stray upstream crash.
            exclude: /node_modules/
            }
        ]
  }
}

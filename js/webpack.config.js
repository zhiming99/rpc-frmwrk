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
}

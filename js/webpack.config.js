// webpack.config.js
let path = require('path')

const webpack=require('webpack')
module.exports = {
    mode:'development',
    entry: './src/index.js',
    output:{
        filename:'rpc-frmwrk.js',
        path:path.resolve(__dirname,'dist'),
    },
    plugins: [
        new webpack.ProvidePlugin({
        process: 'process/browser',
        }),
    ],
}

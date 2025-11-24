let path = require( 'path' );
const webpack = require( 'webpack' );
module.exports = 
{
    // replace 'development' with 'production' for a human-unreadable version
    mode: 'production',
    entry: './maincli.js',
    resolve:{
        modules: [path.resolve(__dirname,
            'node_modules'), 'node_modules'],
        alias:{
            stream: require.resolve('stream-browserify'),
        },
    },
    output:
    {
        path:path.resolve(__dirname, 'dist'),
        filename: 'appmon.js',
        globalObject: 'this',
        library:
        {
            name: 'appmon',
            type: 'var',
        }
    },
    plugins: [
        new webpack.ProvidePlugin(
        {
            process: 'process/browser',
        }),
        new webpack.ProvidePlugin(
        {
            Buffer: ['buffer', 'Buffer'],
        }),
    ],
}
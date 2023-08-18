module.exports = {
  entry: "./client.ts",
  output: {
    filename: "everything.js"
  },
  performance: { hints: false},
  mode: "development",

  // Enable sourcemaps for debugging webpack's output.
  devtool: "source-map",

  resolve: {
    // Add '.ts' and '.tsx' as resolvable extensions.
    // Note: we're not using tsx.
    extensions: [".js", ".jsx", ".ts", ".tsx"],
    alias: {
       vue: "vue/dist/vue.esm-bundler.js"
    },
  },

  module: {
    rules: [
      // All files with a '.ts' or '.tsx' extension will be handled by 'ts-loader'.
      { test: /\.tsx?$/, use: "ts-loader", exclude: /node_modules/ }
    ]
  },
};

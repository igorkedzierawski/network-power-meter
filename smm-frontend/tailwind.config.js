module.exports = {
    content: [
        "./src/**/*.{html,ts}",
    ],
    theme: {
        extend: {
            screens: {
              'custom-lg': '1240px',
            },
            width: {
              '1000': '1000px',
              '580': '580px',
            },
        },
    },
    plugins: [],
}
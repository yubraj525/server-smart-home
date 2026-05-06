/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,jsx}"],
  theme: {
    extend: {
      boxShadow: {
        soft: "0 18px 55px rgba(15, 23, 42, 0.12)",
        glow: "0 18px 50px rgba(250, 204, 21, 0.32)",
      },
      colors: {
        ink: "#0f172a",
      },
    },
  },
  plugins: [],
};

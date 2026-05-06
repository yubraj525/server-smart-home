const config = {
  port: Number(process.env.PORT || 4000),
  allowedOrigin: process.env.CORS_ORIGIN || "*",
  websocketPath: "/ws",
};

module.exports = config;

let pomelo;
try {
    // Try to load native
    pomelo = await import("pomelo");
} catch (err) {
    // Try to load from files
    pomelo = await import("../build/libpomelo-udp-quickjs.so");
} 

export const ChannelMode = pomelo.ChannelMode;
export const ConnectResult = pomelo.ConnectResult;
export const Message = pomelo.Message;
export const Socket = pomelo.Socket;
export const Plugin = pomelo.Plugin;
export const Token = pomelo.Token;
export const Platform = pomelo.Platform;
export const statistic = pomelo.statistic;

import testToken from "./token-test.js";
import testMessage from "./message-test.js";
import testSocket from "./socket-test.js";
import testSTD from "./std-test.js";
import { statistic } from "pomelo";


async function test() {
    let ret;
    ret = testToken();
    console.log(`Test token: ${ret ? "OK" : "Failed"}`);

    ret = testMessage();
    console.log(`Test message: ${ret ? "OK" : "Failed"}`);

    ret = testSocket();
    console.log(`Test socket: ${ret ? "OK" : "Failed"}`);

    ret = await testSTD();
    console.log(`Test std: ${ret ? "OK" : "Failed"}`);

    // Check statistic
    const stat = convertBigInt(statistic());
    console.log(JSON.stringify(stat, null, 2));
}


function convertBigInt(object) {
    const result = {};
    for (let key in object) {
        if (typeof object[key] === "bigint") {
            result[key] = Number(object[key]);
        } else if (typeof object[key] === "object") {
            result[key] = convertBigInt(object[key]);
        } else {
            result[key] = object[key];
        }
    }
    return result;
}


test().catch(err => { console.error(err); });

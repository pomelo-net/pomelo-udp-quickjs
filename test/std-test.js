

function testSetTimeout() {
    return new Promise((resolve) => {
        console.log("SetTimeout: ", Date.now());
        setTimeout(() => {
            console.log("SetTimeout callback: ", Date.now());
            resolve(true);
        }, 100);
    });
}


function testSetInterval() {
    let count = 0;
    return new Promise((resolve) => {
        console.log("SetInterval: ", Date.now());
        let interval;
        interval = setInterval(() => {
            console.log("SetInterval callback: ", Date.now());
            count++;
            if (count >= 5) {
                console.log("SetInterval done: ", Date.now());
                clearInterval(interval);
                resolve(true);
            }
        }, 100);
    });
}


export default async function testSTD() {
    await testSetTimeout();
    await testSetInterval();
    return true;
}

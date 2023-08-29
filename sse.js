function connectToSource() {
    const source = new EventSource("http://localhost:8080/ooga_booga");

    source.onopen = () => {
        console.log("connected to source, about to listen");
        listenToSSE(source);
    };

    source.onerror = (error) => {
        console.error("error occurred:", error);
        source.close();
    };
}

function listenToSSE(source) {
    source.onmessage = (event) => {
        console.log("message received:", event.data);
    };

    source.addEventListener("JS_ELEMENT_RUN", (event) => {
        const data = JSON.parse(event.data);

        const target = data.target;
        const positions = data.positions;
        const callfront = data.callfront;

        WS_ELEMENT_SET(target, positions, eval(callfront));
    });
}

connectToSource();

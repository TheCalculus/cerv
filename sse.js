function connectToSource() {
    const source = new EventSource("[EVENT-SOURCE]");
    if (source.readyState !== 1) return;

    listenToSSE(source);
    source.close();
}

function listenToSSE(source) {
    source.onmessage = (event) => { console.log(event, event.data); };

    source.addEventListener("JS_ELEMENT_RUN", (event) => {
        const data      = JSON.parse(event.data);

        const target    = data.target;           // selector to... select the target
        const callfront = data.callfront;        // javascript function to execute
        const positions = data.positions;        // provides front and back of content that has to be replaced

        WS_ELEMENT_SET(target, template, eval(callfront));
    });
}

connectToSource();

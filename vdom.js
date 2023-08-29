String.prototype.replaceAt = function(indexA, indexB, replacement) {
    return this.substring(0, indexA) + replacement + this.substring(indexB + 1);
}

function WS_ELEMENT_SET(elementSelector, positions, content) {
    const elem     = document.querySelector(elementSelector);
    let   elemCont = elem.innerHTML;

    elemCont       = elemCont.replaceAt(positions[0], positions[1], content);
    elem.innerHTML = elemCont;
}

// WS_ELEMENT_SET("#target", [0, 3], "<span style='color: orange !important;'>ooga booga</span>");

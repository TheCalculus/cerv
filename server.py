from flask import Flask, Response
from flask_cors import CORS
import time
import json

app = Flask(__name__)
CORS(app)

def generate_sse():
    count = 0
    while True:
        data = {
                "target": "#target",
                "positions": [0, 3],
                "callfront": count
                }

        yield "event: JS_ELEMENT_RUN\n"
        yield f"data: {json.dumps(data)}\n\n"
        
        count += 1
        time.sleep(1)

@app.route('/ooga_booga')
def sse_route():
    return Response(generate_sse(), mimetype='text/event-stream')

if __name__ == '__main__':
    app.run(host='localhost', port=8080)

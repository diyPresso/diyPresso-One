#!/usr/bin/python3
from flask import Flask, request, send_from_directory
import time, os, json, sys, traceback, datetime

PORT=8888

# set the project root directory as the static folder, you can set others.
app = Flask(__name__, static_url_path='')

@app.route('/fw/<path:path>')
def send_js(path):
    print("Send file", path)
    return send_from_directory('fw', path)


@app.route('/api/<path:path>', methods = ['GET', 'POST', 'DELETE'])
def data(path):
    print('\n\nNew data request', path)
    #print(request.method)
    #print(request.files)

    if request.method == 'GET':
        """return the information for this camera: Not supported"""
        pass
        
    elif request.method == 'POST':
        print("POST received, %d files" % (len(request.files)))
        print(request.files)

        print("POST received, %d form items" % (len(request.form)))
        print(request.form)

        print("POST received, %d data bytes" % (len(request.data)))        
        print(request.data)
    
        try:
            os.remove('Report')
        except Exception as e:
            print(str(e))
            
        for f in request.files:
            print(f"filename: {request.files[f].name}")
            request.files[f].save(f)
            if request.files[f].name != "Image":
                with open(request.files[f].name) as f:
                    d = f.read()
                    print(str(d))
                    try:
                        print(str( json.loads(d) ))
                    except:
                        print("\07\033[31;1mERROR:JSON invalid")
                        traceback.print_exc() 
                        print("\033[0m")
        time.sleep(1)
        return app.response_class(response="{}",status=200, mimetype='application/json')

    elif request.method == 'DELETE':
        """delete user with ID <user_id>"""

    else:
    
        # POST Error 405 Method Not Allowed
        print("405")
    return app.response_class(response="",status=405)


if __name__ == "__main__":
    app.run(port=PORT, host='0.0.0.0')

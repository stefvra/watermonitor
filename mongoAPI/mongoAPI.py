import datetime
from flask import Flask
from flask import request
from pymongo import MongoClient
from bson.json_util import dumps
import json

import _secrets 

client = MongoClient(_secrets.mongo_url)
database = 'mongoAPI'
collection = 'test'

app = Flask(__name__)

@app.route("/add", methods = ['POST'])
def add():
    try:
        print(request.data)
        data = json.loads(request.data)
        data['date'] = datetime.datetime.now()
        print(data)
        client[database][collection].insert_one(data)
        return dumps({'message' : 'SUCCESS'})
    except Exception as e:
        print(e)
        return dumps({'error' : str(e)})

if __name__ == '__main__':
  
    # run() method of Flask class runs the application 
    # on the local development server.
    app.run(host='0.0.0.0', port=_secrets.serverport, debug=True)
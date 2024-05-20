#!/usr/bin/env python3

import requests 

# Local test influx server 
INFLUX_TOKEN="Di8XWkE1Qz_kx0clc5TLXDfQhLVF2USrFBOq5Sh_cPLHqNH0JaJaP_Q6isIpqclOlWDcE04sMuxsQDsLIvLKDQ=="
INFLUX_HOST="http://192.168.123.110:8086"
INFLUX_ORG="peter"
INFLUX_BUCKET="diyPresso"

class InfluxClient():
    def __init__(self):
        self.data = ""

    def write(self,measurement="measurement_name", tags={}, fields={}, time=None):
        tag_list=""
        field_list=""
        for k,v in tags.items():
            if len(tag_list):
                tag_list += ","
            tag_list += k + "=" + str(v)   
        for k,v in fields.items():
            if len(field_list):
                field_list += ","
            field_list += k + "=" + str(v)
        data = f"{measurement},{tag_list} {field_list}"
        if time != None:
            data += " " + str(int(time))
        if len(self.data):
            self.data += "\n"
        self.data += data

    def write_line(self, data):
        self.data += data + "\n"

    def send(self):
        url = f"{INFLUX_HOST}/api/v2/write?org={INFLUX_ORG}&bucket={INFLUX_BUCKET}&precision=ns"

        headers = {
            "Authorization": f"Token {INFLUX_TOKEN}",
            "Content-Type": "text/plain; charset=utf-8",
            "Accept": "application/json"
        }
        print(self.data)

        response = requests.post(url, headers=headers, data=self.data)

        if response.status_code == 204:
            print("Data posted successfully")
            self.data = ""
        else:
            print("Failed to post data")
            print("Status Code:", response.status_code)
            print("Response:", response.text)
            response.close()


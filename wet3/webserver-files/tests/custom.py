from codecs import raw_unicode_escape_decode
from math import ceil
import random
from signal import SIGINT
from time import sleep
import pytest
import psutil
import os
from server import Server, server_port
import requests
from utils import *
from definitions import *
SINGLE_FILES = {'/home.html': [True, STATIC_OUTPUT_CONTENT, generate_static_headers(r"\d+", r"\d+", r"\d+", r"\d+", "text/html")],
                '/favicon.ico': [False, None, generate_static_headers(r"\d+", r"\d+", r"\d+", r"\d+", "text/plain")]
                }


if __name__ == "__main__":
    with Server(f"{os.getcwd()}/server", 8003, 1, 30, "block") as server:
        sleep(0.1)
        for _ in range(20):
            for file_name, options in SINGLE_FILES.items():
                clients = []
                for _ in range(25):
                    session = FuturesSession()
                    clients.append((session, session.get(f"http://localhost:{8003}/{file_name}")))
                for client in clients:
                    response = client[1].result()
                    client[0].close()
                    expected = options[1]
                    expected_headers = options[2]
                    print(response.headers)
                    if options[0]:
                        validate_response_full(response, expected_headers, expected)
                    else:
                        validate_response_binary(response, expected_headers, expected)
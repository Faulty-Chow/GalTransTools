import re
import sys
import os
import tiktoken
from openai import OpenAI, OpenAIError
import pandas as pd
from typing import List
import json
from datetime import datetime

MODEL = "gpt-4o-mini"
ENC = tiktoken.get_encoding("cl100k_base")
MAX_INPUT = 3000
MAX_SUMMARY = 300
TEMPERATURE = 0.0


class processer:
    def __init__(self, api_key):
        if not os.path.exists(api_key):
            raise FileNotFoundError(f"API key file not found: {api_key}")
        with open(api_key, 'r', encoding='utf-8') as f:
            key = f.read().strip()
        self.client = OpenAI(api_key=key)
        self.game_intro = None
        self.orig = None
        self.trans: List[dict] = []

        self.current_summary = None
        self.current_tokens = 0
        self.current_chunk: List[dict] = []

    def load_game_introduction(self, path: str):
        with open(path, 'r', encoding='utf-8') as f:
            self.game_intro = f.read().strip()

    def translate_chunk(self):
        sys_msg = (
            "You are a professional game translator. "
            "Translate game text into natural and accurate Chinese, "
            "preserving the original meaning, tone, and context. "
            "Avoid literal translations and ensure the result reads fluently to native players.\n\n"
        )
        if self.game_intro:
            sys_msg += f"Game Introduction:\n{self.game_intro}\n\n"
        if self.current_summary:
            sys_msg += f"Previous Local Summary:\n{self.current_summary}\n\n"

        user_msg = (
            "Translate the following JSON array of game text into Chinese. "
            "Return a JSON array with the same structure, "
            "ensuring a strict one-to-one correspondence with the original. "
            "Do not include any additional comments or formatting.\n"
            f"```json\n{json.dumps(self.current_chunk, ensure_ascii=False, indent=2)}\n```"
        )

        resp = self.client.chat.completions.create(
            model="gpt-4o-mini",
            messages=[
                {"role": "system", "content": sys_msg},
                {"role": "user", "content": user_msg}
            ],
            temperature=0.0,
            max_tokens=len(self.current_chunk) * 50
        )

        content = resp.choices[0].message.content.strip()
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        log = {
            "timestamp": timestamp,
            "request": {
                "system": sys_msg,
                "user": user_msg
            },
            "response": content
        }
        os.makedirs("api_logs", exist_ok=True)
        filename = os.path.join("api_logs", f"chat_{timestamp}.json")
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(log, f, ensure_ascii=False, indent=2)

        return content

    def summarize_chunk(self):
        sys_msg = (
            "You are a professional game localization assistant. "
            "Your task is to analyze game text and generate a concise summary that captures its theme, tone, and context. "
            "This summary will be used to guide accurate and context-aware translation.\n\n"
        )
        if self.game_intro:
            sys_msg += f"Game Introduction:\n{self.game_intro}\n\n"
        if self.current_summary:
            sys_msg += f"Previous Local Summary:\n{self.current_summary}\n\n"

        user_msg = (
            "Summarize the following JSON array of game text into a concise and informative description. "
            "Limit the summary to no more than 300 characters.\n"
            f"```json\n{json.dumps(self.current_chunk, ensure_ascii=False, indent=2)}\n```"
        )

        resp = self.client.chat.completions.create(
            model="gpt-4o-mini",
            messages=[
                {"role": "system", "content": sys_msg},
                {"role": "user", "content": user_msg}
            ],
            temperature=0.0,
            max_tokens=500
        )

        summary = resp.choices[0].message.content.strip()
        return summary

    def process(self, item):
        item_str = json.dumps(item, ensure_ascii=False, indent=2)
        item_tokens = len(ENC.encode(item_str))

        if self.current_tokens + item_tokens > MAX_INPUT:
            while True:
                try:
                    self.request()
                    break
                except Exception as e:
                    print(f"请求失败：{e}")
                    retry = input("是否重试？(y/n): ").strip().lower()
                    if retry != 'y':
                        raise

        self.current_chunk.append(item)
        self.current_tokens += item_tokens

    def request(self):
        reply = self.translate_chunk()

        m = re.search(r'```(?:json)?\s*(\[\s*[\s\S]*?\])\s*```', reply)
        json_text = m.group(1) if m else reply
        json_text = json_text.strip()
        translated = json.loads(json_text)
        if not isinstance(translated, list):
            raise ValueError("Response is not a JSON array.")
        translated = self.match_trans(self.current_chunk, translated)
        if len(translated) != len(self.current_chunk):
            raise ValueError(f"Item count mismatch: expected {len(self.current_chunk)}, got {len(translated)}")

        self.trans.extend(translated)
        self.current_summary = self.summarize_chunk()

        self.current_chunk = []
        self.current_tokens = 0

    def match_trans(self, origin, trans):
        from PyQt6.QtCore import QJsonDocument
        from PyQt6.QtNetwork import QTcpSocket, QAbstractSocket
        socket = QTcpSocket()
        socket.connectToHost("localhost", 12345)

        if not socket.waitForConnected(5000):
            print("Failed to connect to the server.")
            return trans

        request = {
            "origin": origin,
            "trans": {
                "label": MODEL,
                "data": trans
            }
        }
        socket.write(json.dumps(request).encode('utf-8'))
        if not socket.waitForReadyRead(3000):
            print("No reply from the server.")
            return trans
        reply = socket.readAll().data().decode('utf-8')
        print("Server reply:", reply)

        reply = b""
        while socket.state() == QAbstractSocket.SocketState.ConnectedState:
            if not socket.waitForReadyRead(10000):
                print("⏳ No data in 10s, still waiting...")
                continue
            reply += socket.readAll().data()
            try:
                obj = json.loads(reply.decode("utf-8"))
                trans = obj.get("trans", [])
                print("✅ Accepted from server.")
                break
            except json.JSONDecodeError as e:
                print(f"⚠️ JSON 未完成或格式错误: {e}. 等待更多数据...")

        socket.close()
        return trans

    def load_data(self, input: str):
        with open(input, 'r', encoding='utf-8-sig') as f:
            data = json.load(f)
        if not isinstance(data, list):
            raise ValueError("Input data must be a JSON array.")

        self.orig = data
        for item in self.orig:
            self.process(item)

        if self.current_tokens > 0:
            self.request()

    def save_data(self, output: str):
        with open(output, "w", encoding="utf-8-sig") as f:
            json.dump(self.trans, f, ensure_ascii=False, indent=2)


if __name__ == '__main__':
    input_data = 'data/orig/00000001.csv.json'
    output_data = 'data/trans/00000001.csv.json'
    game_intro_path = 'data/game_intro.txt'
    api_key = 'data/openai_key'

    processor = processer(api_key)
    processor.load_game_introduction(game_intro_path)
    processor.load_data(input_data)
    processor.save_data(output_data)

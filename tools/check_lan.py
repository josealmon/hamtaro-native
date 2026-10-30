"""Exercise original Ham-Jam trades in two independent native processes.

Requires a private save with Ham-Jam and the first two songs available. All inputs
are ordinary buttons; observations are read-only. Never writes the input save.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path


class Worker:
    def __init__(self, exe: Path, rom: Path, save: Path, profile: Path):
        self.profile = profile
        self.process = subprocess.Popen([str(exe), str(rom), str(save), str(profile)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, creationflags=subprocess.CREATE_NO_WINDOW)
        self.receive()

    def send(self, command: str):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def receive(self):
        line = self.process.stdout.readline()
        if not line:
            raise RuntimeError(self.process.stderr.read())
        result = json.loads(line)
        if isinstance(result, dict) and result.get("fault"):
            raise RuntimeError(f"Native fault: {result}")
        return result

    def ask(self, command: str):
        self.send(command)
        return self.receive()

    def read(self, address: int, count: int):
        return bytes(self.ask(f"R {address:x} {count}"))

    def close(self):
        if self.process.poll() is None:
            self.send("Q")
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        assert self.process.returncode == 0


def advance(workers, sequence):
    for count, keys in sequence:
        for worker in workers:
            worker.send(f"F {count} {keys}")
        states = [worker.receive() for worker in workers]
    return states


def wait_network(workers):
    port = workers[0].ask("H 0")["port"]
    workers[1].ask(f"J 127.0.0.1 {port}")
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        if all(w.ask("S")["net"] == 3 for w in workers):
            return
        time.sleep(.01)
    raise RuntimeError("LAN handshake failed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--save", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=False)
    source_hash = hashlib.sha256(args.save.read_bytes()).hexdigest()
    report = {"source_sha256": source_hash, "cases": []}
    for mode in ("two-way", "one-way", "disconnect", "cancel"):
        workers = []
        try:
            for i in range(2):
                workers.append(Worker(args.worker, args.rom, args.save, args.work / f"{mode}-{i}"))
            wait_network(workers)
            advance(workers, [(1200, 0), (1, 8), (240, 0), (1, 128), (30, 0), (1, 1), (240, 0)])
            # Select different songs to prove the data travelled between processes.
            advance([workers[1]], [(1, 128), (30, 0)])
            advance(workers, [(1, 1), (150, 0), (1, 1), (120, 0), (1, 1), (180, 0),
                (1, 128), (20, 0), (1, 128), (20, 0), (1, 128), (20, 0), (1, 1), (180, 0)])
            offered = [w.read(0x03003D30, 128) for w in workers]
            assert offered[0] != offered[1], "The fixture must provide two different dances"
            before = [w.profile.joinpath("game.sav").read_bytes() for w in workers]
            if mode == "one-way":
                advance([workers[1]], [(1, 128), (20, 0)])
            states = advance(workers, [(1, 1), (180, 0)])
            assert all(s["phase"] == 4 and s["net"] == 3 for s in states), states
            if mode in ("disconnect", "cancel"):
                if mode == "disconnect":
                    workers[0].ask("D")
                    time.sleep(.05)
                    states = advance(workers, [(480, 0)])
                else:
                    states = advance([workers[0]], [(1, 2), (480, 0)])
                    states += advance([workers[1]], [(480, 0)])
                assert all(w.profile.joinpath("game.sav").read_bytes() == prior for w, prior in zip(workers, before))
                assert all(s["result"] == 2 and s["main"] != 0x080D98EE for s in states), states
                report["cases"].append({"mode": mode, "saves_unchanged": True, "states": states})
                print(f"{mode}: passed", flush=True)
                continue
            states = advance(workers, [(1, 1), (240, 0)])
            assert all(s["result"] == 1 for s in states), states
            recipients = [0, 1] if mode == "two-way" else [1]
            for i in recipients:
                actual = workers[i].read(0x03003D30, 128)
                assert actual[0] == 3 and actual[1:] == offered[1-i][1:]
            for w in workers:
                w.ask("P")
            advance(workers, [(1, 1), (180, 0)])
            receivers = [workers[i] for i in recipients]
            advance(receivers, [(1, 128), (20, 0), (1, 1), (300, 0), (1, 1), (180, 0)])
            records = {}
            for i in recipients:
                w = workers[i]
                row = 1-i
                context = int.from_bytes(w.read(0x03002BB0, 4), "little")
                record = w.read(context + row*1100 + 220, 128)
                assert record[0] == 3 and record[17:] == offered[1-i][17:]
                records[i] = (row, record)
                assert w.profile.joinpath("game.sav").read_bytes() != before[i]
            if mode == "one-way":
                assert workers[0].profile.joinpath("game.sav").read_bytes() == before[0]
            for w in workers:
                w.close()
            # Reload saved data in fresh processes and enter Ham-Jam normally.
            for i, (row, expected) in records.items():
                reload = Worker(args.worker, args.rom, workers[i].profile / "game.sav", args.work / f"{mode}-reload-{i}")
                try:
                    advance([reload], [(1200, 0), (1, 8), (240, 0), (1, 128), (30, 0), (1, 1), (240, 0)])
                    context = int.from_bytes(reload.read(0x03002BB0, 4), "little")
                    assert reload.read(context + row*1100 + 220, 128) == expected
                finally:
                    reload.close()
            report["cases"].append({"mode": mode, "states": states, "records_match": True, "saved_and_reloaded": True})
        finally:
            for worker in workers:
                worker.close()
        print(f"{mode}: passed", flush=True)
    assert hashlib.sha256(args.save.read_bytes()).hexdigest() == source_hash
    (args.work / "proof.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Two-process Cable Link gameplay checks passed.")


if __name__ == "__main__":
    main()

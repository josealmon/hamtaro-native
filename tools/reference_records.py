"""Private record/counter comparisons with native 32-bit result layouts."""
from __future__ import annotations
import ctypes as C
import random
import struct


def check_record_messages(*,entries,rom,memory,bind,call,both,fresh,equal,compare,command):
    """Complete message family: live text aliases, casing and void presets."""
    from pathlib import Path
    import re
    from inventory_reference import body
    rows=[r for r in entries if r['source']=='src/text/game_record_messages.c']
    equal(len(rows),12,'complete game-record message source family')
    main_address=0x080247A4;tasks={r['address'] for r in rows if r['address']!=main_address}
    source=(Path(__file__).resolve().parents[1]/'src/game/record_menu.c').read_text()
    main_cases={int(v,16) for v in re.findall(r'case (0x[0-9A-Fa-f]+):',body(source,'hn_record_menu_callback'))}
    task_cases={int(v,16) for v in re.findall(r'case (0x[0-9A-Fa-f]+):',body(source,'hn_record_menu_task'))}
    equal({main_address}|tasks,{r['address'] for r in rows},'all twelve source contracts accounted for')
    equal({main_address}<=main_cases and tasks<=task_cases,True,'Main and void message bindings')
    u=C.c_uint32;main=bind('hn_scene_callback',C.c_int,C.c_void_p,u,u,C.POINTER(u))
    task=bind('hn_record_menu_task',C.c_int,C.c_void_p,u)
    task_command=bind('hn_probe_task_command',C.c_int,C.c_void_p,u,C.c_uint,C.POINTER(u))
    rng=random.Random(0x5245434D5347);controls=0x02004520;args=0x0203F001
    buffer=0x03003B00;catalog=0x02018000;text=0x02019000;base=0x02008000
    ranges=((0x02000000,0x40000),(0x03000000,0x7000))
    sentence=b'\x61\x01\x62\x01\x92\x01\x93\x0C\x01\xFF\x61'
    def byte(a,v):both(a,bytes([v&255]))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def fixture(record=0,language=0,kind=0,payload=sentence,input_address=args,source_address=text):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(controls,rng.randbytes(512))
        both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,255)
        both(buffer-64,rng.randbytes(640));both(0x0201DC40,rng.randbytes(512))
        byte(0x03001F50,record);byte(0x03002170,record^0xA5)
        for table in (0x02000FC0,0x02001104,0x02001248):both(table,struct.pack('<I',catalog)*256)
        both(catalog,struct.pack('<I',source_address)*256)
        both(source_address,payload+b'\0');byte(controls+9,record);byte(controls+0xEB,language)
        byte(input_address,kind)
    def exercise(label,input_address=args,vm=False):
        if vm:
            byte(0x03000010,0x1C);equal(command(memory,0x1C,input_address),1,'record-message Main command')
            call('MainScriptInvokeCallback',input_address,limit=1000000)
        else:
            consumed=u(99);equal(main(memory,main_address|1,input_address,C.byref(consumed)),1,'message native Main '+str(label))
            equal(consumed.value,call('ScriptCallbackPrepareGameRecordMessageText',input_address,limit=1000000),'message argument consumption')
        compare('message '+str(label),ranges)
    for kind in range(3):
        for language in range(5):
            for record in range(256):
                fixture(record,language,kind);exercise(('record/language/type',record,language,kind))
    # Read real bundles from the player's initialized tables, including defaults.
    # Original table extents: 31 type-0 bundles, then 81 for each other type.
    for kind,(table,count) in enumerate(((0x02001248,31),(0x02001104,81),(0x02000FC0,81))):
        initial=rom[0x8FE018:0x900EC8]
        for record in range(count):
            original=struct.unpack_from('<I',initial,table-0x02000000+record*4)[0]
            equal(0x08000000<=original<0x09000000,True,'original message bundle address')
            for language in range(5):
                fixture(record,language,kind);word(table+record*4,original);exercise(('ROM bundle',record,language,kind))
    for kind,table in enumerate((0x02001248,0x02001104,0x02000FC0)):
        for language in range(5):
            fixture(7,language,kind);word(table+7*4,0x0858C6B0);exercise(('default text script',kind,language))
    for kind in range(3):
        for language in range(5):
            for length in (0,1,2,31,32,254,255,256,257,511,512):
                fixture(7,language,kind,(b'\x61\x01\x92'*171)[:length]);exercise(('byte index wrap',kind,language,length))
    for offset in (-40,-1,0,1,32,255,300):
        for kind in range(3):
            for language in (0,3):
                fixture(7,language,kind,sentence*4,source_address=buffer+offset);exercise(('text/output alias',offset,kind,language))
    for input_address in (buffer-1,buffer,buffer+1,controls+9,controls+0x69,controls+0xEB):
        for kind in range(3):
            fixture(7,3,kind,input_address=input_address);exercise(('live selector alias',input_address,kind),input_address)
    fixture(7,3);both(args,struct.pack('<I',main_address|1)+b'\0');exercise('Main VM',vm=True)
    for row in rows:
        if row['address']==main_address:continue
        for selected in range(256):
            fixture(selected);equal(task(memory,row['address']|1),1,row['name']+' native void')
            call(row['name']);compare(row['name']+' follower alias/timer',ranges)
        fixture(19);consumed=u(99)
        equal(main(memory,row['address']|1,args,C.byref(consumed)),0,'void message preset rejected by Main')
        compare('void message ABI rejection',ranges)
        both(0x0201F000,rng.randbytes(52));word(0x0201F010,args);both(args,b'\x01\xE1')
        word(0x03001FC4,row['address']|1);report=(u*4)()
        equal(task_command(memory,0x0201F000,0xF4,report),1,'message actual F4 command')
        equal(report[3],call('TaskCommandF4',0x0201F000),'message F4 continuation');compare('message F4',ranges)
    fixture(7);equal(task(memory,main_address|1),0,'argument-dependent message rejected as void');compare('message void ABI rejection',ranges)
    for kind in (3,127,255):
        fixture(7,0,kind);consumed=u(99)
        equal(main(memory,main_address|1,args,C.byref(consumed)),2,'undefined message selector fails explicitly')
    print('All 12 game-record message functions matched: five languages, special initials, live aliases, wrapped text indices and void/F4 presets.')


def check_record_counter_prefix(*,entries,memory,bind,call,both,fresh,equal,compare,command):
    """Reviewed Main51 assembly leaf; index 31 is read but never counted."""
    rows=[r for r in entries if r['source']=='asm/save_counter_prefix.s']
    equal([(r['name'],r['address']) for r in rows],[('StoreNonemptySaveGroup01CountAsm',0x0802C874)],'complete counter-prefix assembly family')
    rng=random.Random(0x434E5450524546);args=0x0203F001;counter=0x0201CC20
    ranges=((0x02000000,0x40000),(0x03000000,0x7000))
    u=C.c_uint32;main=bind('hn_scene_callback',C.c_int,C.c_void_p,u,u,C.POINTER(u))
    for position in range(32):
        for value in range(256):
            fresh();values=bytearray(rng.choices((0,0,1,2,127,254,255,255),k=32));values[position]=value
            both(counter,bytes(values));both(0x02004520,rng.randbytes(512));both(0x03000010,b'\x51'+bytes(15))
            both(args,struct.pack('<I',0x0802C875));count=sum(v not in (0,255) for v in values[:31])
            equal(call('StoreNonemptySaveGroup01CountAsm'),count,'counter-prefix original R0')
            equal(command(memory,0x51,args),1,'counter-prefix typed Main51 dispatch')
            call('MainScriptDispatchArgumentWord',args);compare('counter-prefix full byte/index domains',ranges)
    consumed=u(99);equal(main(memory,0x0802C875,args,C.byref(consumed)),0,'counter-prefix rejects ordinary Main ABI')
    compare('counter-prefix Main rejection preserves memory',ranges)
    print('Counter-prefix Main51 leaf matched all 32 positions and all 256 byte values; index 31 remains excluded.')


def check_group4(*,entries,rom,memory,bind,call,both,fresh,equal,compare):
    """Shared offline save-group-4 operations used by the music hall.

    Transport and exchange callbacks remain outside this native contract set.
    """
    names={"ScriptCallbackCopyControlByteToCommunicationContext",
           "ScriptCallbackCopyCommunicationByteToControl","ScriptCallbackSelectCommunicationRecord",
           "ScriptCallbackClassifyCommunicationPayload","ScriptCallbackCheckReceivedCommunicationFlags"}
    rows={row["name"]:row for row in entries if row["name"] in names}
    equal(set(rows),names,"five offline group4 dependency contracts")
    callback=bind("hn_group4_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    controls=0x02004520;received=0x03003D30;context=0x02010000;args=0x02001001
    # Game RAM, as in the existing comparisons. The reference's ARM call stack
    # is outside the host callback's serialized memory contract.
    ranges=((0x02000000,0x40000),(0x03000000,0x7000))
    rng=random.Random(0x48334734)
    descriptors=[(struct.unpack_from("<I",rom,0x6E8CCC+row*28+8)[0],rom[0x6E8CCC+row*28+0x16]) for row in range(14)]
    categories=[value-0x45 for value in rom[0x6E906A:0x6E9074]]
    def fixture(row):
        fresh();both(controls,rng.randbytes(512));both(received,rng.randbytes(512))
        both(context,rng.randbytes(0x5000));both(0x03002BB0,struct.pack("<I",context))
        both(0x03002180,rng.randbytes(96));both(controls+0x180,bytes([row]))
    def exercise(name):
        consumed=C.c_uint32(0xFFFF)
        equal(callback(memory,rows[name]["address"]|1,args,C.byref(consumed)),1,name+" native Main ABI")
        equal(consumed.value,call(name,args),name+" argument consumption")
        compare(name+" EWRAM/game IWRAM",ranges)
    for name in ("ScriptCallbackCopyControlByteToCommunicationContext","ScriptCallbackCopyCommunicationByteToControl"):
        for offset in (0,0x36,0x1FF,0xFFFF):
            for value in range(256):
                fixture(0);both(args,struct.pack("<H",offset))
                both(context+0x2B18,bytes([value]));both(controls+offset,bytes([value^0xA5]))
                exercise(name)
    for row in range(14):
        for column in range(256):
            fixture(row);both(controls+0x181,bytes([column]));exercise("ScriptCallbackSelectCommunicationRecord")
    for offset in (-64,-1,0,1,16,17,64):
        fixture(0);both(controls+0x181,b"\0");both(0x03002BB0,struct.pack("<I",received+offset))
        both(received-64,rng.randbytes(640));exercise("ScriptCallbackSelectCommunicationRecord")
    for row,(data,length) in enumerate(descriptors):
        original=rom[data-0x08000000:data-0x08000000+length]
        equal(len(original),length,"group4 descriptor points into the player ROM")
        patterns=[bytes(length),bytes([1])*length,original,bytes([0xFE])*length,rng.randbytes(length)]
        if length:patterns.append(bytes(length-1)+b"\x34")
        patterns.extend(bytes(categories[:n])+bytes(max(0,length-n)) for n in range(11))
        for mode in (0,1,2,3,255):
            for pattern in patterns:
                fixture(row);both(controls+0x60,bytes([mode]));both(received+17,pattern[:length])
                exercise("ScriptCallbackClassifyCommunicationPayload")
        for kind in range(8):
            fixture((row+1)%14);both(controls+4,bytes([row]))
            pattern=(bytes(length) if kind==0 else bytes([1])*length if kind==1 else
                     bytes([2])*length if kind==2 else bytes([255])*length if kind==3 else rng.randbytes(length))
            both(received+17,pattern)
            if kind in (2,4):both(0x03002180,bytes(96))
            if kind in (3,5):both(0x03002180,bytes([255])*96)
            exercise("ScriptCallbackCheckReceivedCommunicationFlags")
    print("Five shared local group4 callbacks matched; full byte/control and column domains, aliases, flags and classification modes. Link transport remains unsupported.")


def check_records(*, entries, rom, memory, cpu, bind, call, both, fresh,
                  equal, compare, native_bytes, command):
    families = {"src/save/game_records.c", "src/save/game_record_allocator.c",
                "src/save/game_record_allocate_callback.c", "src/scripts/script_callback_allocate_class_game_record.c",
                "src/scripts/script_callback_allocate_record_batch.c"}
    rows = [row for row in entries if row["source"] in families]
    equal(len(rows), 9, "complete record allocation/query/consume/counter families")
    count = bind("hn_records_count", C.c_uint8, C.c_void_p)
    allocate = bind("hn_record_allocate", C.c_int32, C.c_void_p, C.c_uint16)
    query = bind("hn_record_query", C.c_int32, C.c_void_p, C.c_uint8, C.c_uint32)
    consume = bind("hn_record_consume", C.c_int32, C.c_void_p, C.c_uint16, C.c_uint8, C.c_uint32)
    adjust = bind("hn_record_adjust_counter", C.c_uint32, C.c_void_p, C.c_uint8, C.c_int32)
    counter = bind("hn_record_counter", C.c_uint8, C.c_void_p, C.c_uint8)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x5245434F5244)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400))
    slots, args = 0x0201CB20, 0x02020000

    def fixture(case):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(slots, rng.randbytes(4096))
        both(0x0201CC20, rng.randbytes(256))
        both(0x02010000, rng.randbytes(40 * 104))
        both(0x03002D98, struct.pack("<I", 0x02010000))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03001F50, bytes([2 if case & 1 else 13]))
        both(0x03002180, rng.randbytes(96))
        both(0x0201DC40, rng.randbytes(128))
        both(args, rng.randbytes(16))

    def rom_word(address):
        return struct.unpack_from("<I", rom, address - 0x08000000)[0]

    valid_keys = []
    for key in range(128):
        descriptor = rom_word(0x0858CFE8 + key * 4)
        bank = rom_word(0x0858F25C + (descriptor & 3) * 4)
        pointer = bank + ((descriptor >> 2) & 127) * 4
        if 0x08000000 <= pointer <= 0x08FFFFFC or 0x02000000 <= pointer <= 0x0203FFFC:
            valid_keys.append(key)
    equal(bool(valid_keys), True, "record result table fixtures have mapped bank views")
    for case in range(512):
        fixture(case)
        for i in range(64):
            raw = (rng.getrandbits(32) & ~0xF80) | (0 if i < case % 65 else (1 + i % 31) << 7)
            both(slots + i * 4, struct.pack("<I", raw))
        equal(count(memory), call("CountOccupiedGameRecords"), "occupied record count")
        compare("occupied record count RAM", ranges)
        key = (0, 1, 59, 60, 127, 128, 511, 512, 65535)[case % 9]
        equal(allocate(memory, key) & 0xFFFFFFFF, call("AllocateGameRecord", key), "record allocation return")
        compare(f"record allocation {case}", ranges)
    for index in range(256):
        for occupied in (0, 1, 31):
            fixture(index)
            key = valid_keys[index % len(valid_keys)]
            both(slots + index * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) | key | occupied << 7))
            result = (0, args, slots + index * 4)[index % 3]
            both(args, rng.randbytes(12))
            equal(query(memory, index, result), call("QueryGameRecord", index, result), "record query")
            compare("record query/result layout", ranges)
    for case in range(2048):
        fixture(case)
        index = (0, 1, 63, 64, 127, 255, 1023)[case % 7]
        key = valid_keys[case % len(valid_keys)]
        occupied = (0, 1, 2, 30, 31)[case % 5]
        both(slots + index * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) | key | occupied << 7))
        both(0x030021B2, bytes([(case & 1) << 1]))
        result = (0, args, slots + index * 4)[case % 3]
        comparison = (0, 1, 2, 30, 31, 32, 255)[case % 7]
        equal(consume(memory, index, comparison, result) & 0xFFFFFFFF,
              call("ConsumeGameRecord", index, comparison, result), "record consume")
        compare(f"record consume/layout/aliases {case}", ranges)
    for index in range(256):
        fixture(index)
        equal(counter(memory, index), call("QueryGameCounter", index), "game counter index bounds")
        for delta in (0, 1, -1, 127, 128, 255, -255, 2147483647, -2147483648):
            both(0x0201CC20 + index, bytes([(0, 1, 127, 128, 254, 255)[index % 6]]))
            equal(adjust(memory, index, delta), call("AdjustGameCounter", index, delta & 0xFFFFFFFF), "game counter adjust")
            compare("game counter sentinel/wrap/limit", ranges)
    callbacks = [row for row in rows if row["name"].startswith("ScriptCallback")]
    for row in callbacks:
        for case in range(128):
            fixture(case)
            for i in range(64):
                both(slots + i * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) |
                                              valid_keys[i % len(valid_keys)] | ((0 if i == case % 65 else 1) << 7)))
            key = (0, 1, 59, 60, 127, 511, 65535)[case % 7]
            both(0x0201DC84, struct.pack("<H", key))
            both(args, struct.pack("<H", key))
            argument = args if case < 96 else 0x03001137 + case % 3
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, argument, C.byref(consumed)), 1, "record allocate callback")
            equal(consumed.value, call(row["name"], argument), "record allocate callback consumed")
            compare("record allocate callback counter/palette/state", ranges)
        for case in range(16):
            fixture(case)
            both(args, struct.pack("<I", row["address"] | 1) + struct.pack("<H", case))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "record allocation Main ABI")
            call("MainScriptInvokeCallback", args)
            compare("record allocation Main ABI", ranges)
    for case in range(16):
        fixture(case)
        both(0x02019000, rng.randbytes(52))
        both(0x02019010, struct.pack("<I", args))
        both(args, b"\x01\xE1")
        both(0x03001FC4, struct.pack("<I", 0x0801E00D))
        report = (C.c_uint32 * 4)()
        equal(task(memory, 0x02019000, 0xF4, report), 1, "class record allocation F4 ABI")
        equal(report[3], call("TaskCommandF4", 0x02019000), "class record task continuation")
        compare("class record allocation F4 ABI", ranges)
    batch = next(r for r in rows if r["name"] == "ScriptCallbackAllocateGameRecordBatch")
    for occupied in range(65):
        for amount in (0, 1, 2, 30, 31, 32, 33, 63, 64, 65, 255, 65535):
            fixture(occupied + amount)
            for index in range(64):
                both(slots + index * 4, struct.pack("<I", valid_keys[index % len(valid_keys)] | (128 if index < occupied else 0)))
            both(0x0201DC4C, struct.pack("<H", amount));both(args, struct.pack("<H", (0, 6, 11, 63, 65535)[amount % 5]))
            both(0x03002DC0, b"\x01")
            consumed = C.c_uint32(99)
            equal(main(memory, batch["address"] | 1, args, C.byref(consumed)), 1, "batch allocation occupancy/amount edges")
            equal(consumed.value, call(batch["name"], args), "batch allocation consumed")
            compare("batch capacity, zero/full branch, countdown and division boundaries", ranges)
    apply = bind("hn_preset_apply", C.c_int, C.c_void_p, C.c_uint32)
    fixture(0);equal(apply(memory, batch["address"] | 1), 0, "argument-dependent batch rejected by F4")
    compare("rejected record batch preserves memory", ranges)
    print("Nine record allocation/query/consume/counter functions across five families passed; full inventory/save UI integration remains unverified.")

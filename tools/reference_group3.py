"""Private SaveGroup3 contracts; original assets stay in the supplied ROM."""
from __future__ import annotations
import ctypes as C
import random
import struct
from unicorn.arm_const import UC_ARM_REG_SP


def check_group3(*, entries, rom, memory, cpu, bind, call, both, fresh, equal,
                 compare, native_bytes, command):
    rng = random.Random(0x47524F555033)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    base, output, arguments = 0x02008000, 0x0203FF00, 0x0203F000
    u8, u32, ptr = C.c_uint8, C.c_uint32, C.c_void_p
    prep = bind("hn_group3_pair_graphics", C.c_bool, ptr, u8, u32)
    animate = bind("hn_group3_pair_animation", C.c_bool, ptr, u32, u8, u32)
    palette = bind("hn_group3_pair_palette", C.c_bool, ptr, u8, u8, u8, u32)
    number = bind("hn_group3_number", C.c_bool, ptr, u32, u8, u8, u8)
    counts = bind("hn_group3_counts", C.c_bool, ptr)
    selected = bind("hn_group3_selected_count", C.c_bool, ptr)
    field = bind("hn_group3_field_bit", None, ptr, u8, u8, u8)
    remove = bind("hn_group3_remove_list", None, ptr, u8)
    lookup = bind("hn_group3_remove_lookup", C.c_bool, ptr, u8)
    tile_map = bind("hn_group3_tile_map", C.c_bool, ptr, u32, u32, C.c_uint16)
    insert = bind("hn_group3_insert_pair", C.c_bool, ptr, u32, u32)
    position = bind("hn_group3_position", C.c_bool, ptr, u8, u32, u32, u32, u8)
    find = bind("hn_group3_find_pair", C.c_bool, ptr, u32, u8, u8, C.POINTER(u8))
    pair_slot = bind("hn_group3_pair_slot", C.c_bool, ptr, u32, u32, u32, u32, u32, u32)
    scan = bind("hn_group3_scan_pairs", C.c_bool, ptr, u32, u32, u32, u32, C.POINTER(u32))
    task = bind("hn_group3_task", C.c_int, ptr, u32)

    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        both(base, rng.randbytes(256 * 104));both(0x03002D98, struct.pack("<I", base));both(0x03002DC0, b"\x01")
        both(0x03002182, bytes([128 | (case & 127)]));both(0x0300357B, rng.randbytes(257))
        both(0x03003C50, rng.randbytes(1024));both(0x0201EE40, bytes(rng.randrange(82) for _ in range(32)))
        both(0x02004520, rng.randbytes(512));both(0x02004590, bytes([case & 255]))
        for index in range(6):both(0x02002230 + index * 4, struct.pack("<I", 0x02018000 + index * 0x4000))
        both(output, rng.randbytes(32));both(arguments, rng.randbytes(16))

    for category in range(256):
        for bank in (0, 1, 2, 255):
            for offsets in ((0, 4, 8), (0, 0, 8), (0, 4, 4), (0, 0, 0)):
                fixture(category);outputs = tuple(output + offset for offset in offsets)
                equal(position(memory, category, *outputs, bank), True, "group3 category position")
                call("GetSaveGroup3PairPosition", category, *outputs, bank)
                compare("positions, fixed-point exceptions and aliased output pointers", ranges)
    for bit in range(256):
        for index in (0, 1, 29, 30, 255):
            for value in (0, 255):
                fixture(bit);field(memory, index, bit, value);call("SetSaveGroup3FieldBit", index, bit, value)
                compare("group3 byte field and ARM shift count", ranges)
    for case in range(256):
        fixture(case);remove(memory, case);call("RemoveSaveGroup3ListEntry", case)
        compare("list shift and unconditional last-byte clear", ranges)
        fixture(case);equal(lookup(memory, case), True, "group3 lookup removal")
        call("RemoveSaveGroup3ListEntryByLookupValue", case)
        compare("lookup includes preceding ROM byte and not-found identifier 81", ranges)
        fixture(case);equal(counts(memory), True, "group3 count slots")
        call("UpdateSaveGroup3CountRenderSlots");compare("count slot positions and decimal animations", ranges)
        fixture(case);equal(selected(memory), True, "group3 selected count slots")
        call("UpdateSaveGroup3SelectedCountRenderSlots");compare("selected count, wrapped group pointer and full slot word", ranges)
        fixture(case)
        equal(palette(memory, case, case, 0xA5, 0xFACE0000 | (case & 1)), True, "group3 palette byte selectors")
        call("CopySaveGroup3PairPalette", case, case, 0xA5, 0xFACE0000 | (case & 1))
        compare("palette prefix sum wraps as a byte", ranges)
    for case in range(64):
        for offset in (-64, -18, -2, 0, 2, 18, 64, 0x2000):
            fixture(case);source = 0x02032000;destination = source + offset;attributes = rng.randrange(65536)
            both(source - 64, rng.randbytes(0x2400))
            equal(tile_map(memory, destination, source, attributes), True, "group3 tile map")
            call("CopySaveGroup3TileMap", destination, source, attributes)
            compare("six rows, nine columns, full attributes and forward overlap", ranges)
    for case in range(256):
        fixture(case);start = 0x02032000
        both(start, rng.randbytes(520));index = (0, 1, 2, 31, 128, 255)[case % 6]
        equal(insert(memory, start + index * 2, start), True, "group3 insertion")
        call("InsertSaveGroup3PairByFirstByte", start + index * 2, start)
        compare("stable pair insertion and variation-byte order", ranges)
    for length in (0, 1, 2, 31, 255, 256, 512):
        for repeat in range(8):
            fixture(repeat);start = 0x02032000
            pairs = bytes(v for i in range(length) for v in (i % 255, i >> 8)) + b"\xFF\xA5"
            both(start, pairs);first, second = (254, 254) if repeat & 1 else ((length - 1) % 255, (length - 1) // 256 & 255)
            result = u8(99);equal(find(memory, start, first, second, C.byref(result)), True, "group3 find pair")
            equal(result.value, call("FindSaveGroup3PairIndex", start, first, second), "pair find byte return and wrapped count")
            compare("pair lookup preserves memory", ranges)
    for category in range(256):
        for bank in (0, 1):
            fixture(category)
            word = bank | 0xCAFE0000
            equal(prep(memory, category, word), True, "group3 pair graphics")
            call("PrepareSaveGroup3PairGraphics", category, word, limit=2000000)
            compare("private LZ77 resources and captured tile queue arguments", ranges)
            fixture(category)
            equal(animate(memory, 255, category, word), True, "group3 pair animation")
            call("ConfigureSaveGroup3PairAnimation", 255, category, word, limit=2000000)
            compare("category boundaries and full-slot animation", ranges)
            fixture(category);retained = rng.getrandbits(32)
            cpu.mem_write(cpu.reg_read(UC_ARM_REG_SP) - 0x1C, struct.pack("<I", retained))
            equal(pair_slot(memory, 0x123400FF, 0xABCD0010, category | 0xFFFF0000, 0xCAFE0001, word, retained), True,
                  "group3 pair slot with explicit retained Z")
            call("ConfigureSaveGroup3PairRenderSlot", 0x123400FF, 0xABCD0010, category | 0xFFFF0000, 0xCAFE0001, word, limit=2000000)
            compare("byte normalization and empty-category original stack word", ranges)
    for value in range(256):
        for slot in (0, 1, 254, 255, 256, 1024):
            fixture(value);x, y = (value * 7) & 255, 255 - value
            equal(number(memory, slot, value, x, y), True, "group3 number renderer")
            call("RenderSaveGroup3Number", slot, value, x, y)
            compare("byte constructor versus word animation/priority indices", ranges)
    for case in range(512):
        fixture(case);fields, pairs = 0x02032000, 0x02033000
        both(fields, rng.randbytes(256))
        if case % 4 == 3:pairs = fields + (case % 8) * 2
        first, group = rng.getrandbits(32), rng.getrandbits(32)
        total = u32(99);equal(scan(memory, fields, pairs, first, group, C.byref(total)), True, "group3 catalog scan")
        equal(total.value, call("ScanSaveGroup3AvailableFieldPairs", fields, pairs, first, group), "catalog scan count")
        compare("ten rows, five variants, byte category and output/input overlap", ranges)
    for address, original in ((0x080CB028, "InitializeSaveDataGroup3Fields"),
                              (0x080CACB8, "UpdateSaveGroup3CountRenderSlots"),
                              (0x080CAD5C, "UpdateSaveGroup3SelectedCountRenderSlots")):
        fixture(1);equal(task(memory, address | 1), 1, "group3 reviewed no-argument task")
        call(original);compare("typed group3 task", ranges)
    copy_rows=bind("hn_transfer_strided_rows",C.c_bool,ptr,u32,u32,u32,u32,u32)
    decompress_rows=bind("hn_transfer_decompress_rows",C.c_bool,ptr,u32,u32,u32,u32,u32)
    heap_init=bind("hn_heap_init",None,ptr)
    equal(sum(r["source"]=="src/graphics/transfer_decompress_rows.c" for r in entries),2,"complete strided transfer family")
    for case in range(96):
        fixture(case);start=0x02031000;destination=start+(-16,-2,0,2,16,4096)[case%6]
        width=(0,1,2,3,8,16)[case%6];rows=case%5;stride=(0,2,8,16,32)[case%5]
        both(start-64,rng.randbytes(8192))
        equal(copy_rows(memory,start,destination,width|0xABCD0000,rows|0x12340000,stride|0x56780000),True,"strided halfword rows")
        call("CopyStridedHalfwordRows",start,destination,width|0xABCD0000,rows|0x12340000,stride|0x56780000)
        compare("halfword row widths, byte strides and forward overlap",ranges)
    for case in range(32):
        fixture(case);heap_init(memory);call("InitializeHeap")
        both(0x02031000,b"\x10\x10\0\0\0ABCDEFGH\0IJKLMNOP")
        rows=(1,2,4,8)[case%4];width=16//rows
        equal(decompress_rows(memory,0x02031000,0x02032000,width|0xABCD0000,rows|0x12340000,32|0x56780000),True,"decompress strided rows")
        call("DecompressStridedRows",0x02031000,0x02032000,width|0xABCD0000,rows|0x12340000,32|0x56780000)
        compare("allocated decompression, strided row copies and heap free",ranges)
    print("SaveGroup3 field/graphics/catalog helper checks passed.")


def check_group3_scenes(*, entries, rom, memory, cpu, bind, call, both, fresh, equal,
                        compare, native_bytes, command, record_menus=False, selection_scene=False):
    import re
    from pathlib import Path
    from inventory_reference import body
    module="record_menu" if record_menus or selection_scene else "save_group3"
    dispatch="hn_record_menu" if record_menus or selection_scene else "hn_group3"
    source = (Path(__file__).resolve().parents[1] / f"src/game/{module}.c").read_text()
    addresses = lambda name: {int(x, 16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):", body(source, name))}
    main_addresses, task_addresses = addresses(dispatch+"_callback"), addresses(dispatch+"_task")
    rows = [r for r in entries if r["address"] in main_addresses]
    equal(len(rows), len(main_addresses), "reviewed group3 scene entry points have reference symbols")
    families = {"src/scripts/save_group3_pair_render_callbacks.c", "src/scripts/save_group3_pair_selection.c",
                "src/graphics/save_group3_pair_graphics.c", "src/save/save_group3_fields.c",
                "src/save/save_group3_pair_catalog_scan.c", "src/save/save_group3_list.c",
                "src/save/save_group3_list_selection.c"}
    helpers = {"ConfigureSaveGroup3PairAnimation", "PrepareSaveGroup3PairGraphics", "CopySaveGroup3PairPalette",
               "RenderSaveGroup3Number", "UpdateSaveGroup3CountRenderSlots", "UpdateSaveGroup3SelectedCountRenderSlots",
               "SetSaveGroup3FieldBit", "RemoveSaveGroup3ListEntryByLookupValue", "CopySaveGroup3TileMap",
               "InsertSaveGroup3PairByFirstByte", "GetSaveGroup3PairPosition", "InitializeSaveDataGroup3Fields",
               "FindSaveGroup3PairIndex", "ConfigureSaveGroup3PairRenderSlot", "ScanSaveGroup3AvailableFieldPairs",
               "RemoveSaveGroup3ListEntry"}
    family_rows = [r for r in entries if r["source"] in families]
    if record_menus:
        family_rows=[r for r in entries if r["source"]=="src/save/record_selection.c"];helpers=set()
        rows=[r for r in rows if r["source"]=="src/save/record_selection.c"]
    if selection_scene:
        family_rows=[r for r in entries if r["source"]=="src/scripts/script_callback_actor_map10_5.c"];helpers=set()
        central=(Path(__file__).resolve().parents[1]/"src/game/scene.c").read_text()
        central_addresses={int(x,16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):",body(central,"hn_scene_callback"))}
        equal({r["address"] for r in family_rows}<=main_addresses|central_addresses,True,"all selection-scene functions have explicit native dispatch")
        rows=family_rows
    equal(len(family_rows), 14 if selection_scene else 18 if record_menus else 67, "complete menu or SaveGroup3 source inventory")
    equal({r["name"] for r in family_rows}, {r["name"] for r in rows} | helpers, "all family entry points covered")
    if selection_scene:
        dependencies=[]
        for source_name in ("script_callback_key_save_marks.c","script_callback_key_save_sequences.c","script_callback_raster_e5_render.c"):
            family=[r for r in entries if r["source"]=="src/scripts/"+source_name]
            equal(len(family),1,"complete selection dependency family "+source_name)
            equal(family[0]["address"] in main_addresses&task_addresses,True,"selection dependency Main and F4 contracts")
            dependencies+=family
        rows+=dependencies
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    apply = bind(dispatch+"_task", C.c_int, C.c_void_p, C.c_uint32)
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    heap_init = bind("hn_heap_init", None, C.c_void_p)
    allocate_task=bind("hn_task_allocate",C.c_uint32,C.c_void_p)
    initialize_map=bind("hn_map_initialize",C.c_bool,C.c_void_p,C.c_uint8,C.c_uint8)
    rng = random.Random(0x4752335343454E45)
    base, controls, args = 0x02008000, 0x02004520, 0x0203F000
    catalogs, selected, counts, fields = (0x03003A50, 0x03003850, 0x03003680), 0x03003790, 0x0300357C, 0x03003C50
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    if record_menus or selection_scene:ranges+=((0x04000000,0x60),(0x040000B0,12))
    def byte(address, value):both(address, bytes([value & 255]))
    def ctrl(offset, value):byte(controls + offset, value)
    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8]);both(base, rng.randbytes(256 * 104))
        heap_init(memory);call("InitializeHeap")
        both(0x03000608, struct.pack("<I", 0x03000610))
        both(0x03002D98, struct.pack("<I", base));byte(0x03002DC0, 1)
        both(0x03003070, struct.pack("<I", 0x02015000));byte(0x03002E10, 1);both(0x02015000, rng.randbytes(32 * 44))
        both(0x03002E08,struct.pack("<I",0x02017000));both(0x02017000,rng.randbytes(8*80))
        both(controls, rng.randbytes(512));both(fields, rng.randbytes(120));both(0x0201EE40, bytes(rng.randrange(81) for _ in range(32)))
        if selection_scene:both(0x03003900,bytes((i*7+case)%45 if i<48 and (i+case)%5 else 255 for i in range(256)))
        both(0x03002180, rng.randbytes(96));byte(0x03002182, 128 | case & 127)
        both(0x03003730, rng.randbytes(96));both(0x03000820, struct.pack("<H", (0, 0x100, 0x200, 0x300)[case % 4]))
        for group, catalog in enumerate(catalogs):
            both(0x02002780 + group * 4, struct.pack("<I", catalog));both(catalog, b"\xFF" * 162)
            pairs = bytes(v for i in range(12) for v in ((i * 3 + group) % 30, i % 5))
            both(catalog, pairs);byte(counts + group, 12)
            byte(selected + group, case % 13);byte(selected + group + 3, (case + 3) % 13)
        for index in range(6):
            byte(0x03003730 + index * 16, 1 + index % 8)
            for item in range(6):
                byte(0x03003731 + index * 16 + item * 2, (item * 3 + item % 3) % 30)
                byte(0x03003732 + index * 16 + item * 2, item % 5)
            both(0x02002230 + index * 4, struct.pack("<I", 0x02018000 + index * 0x4000))
        ctrl(0x60, case % 5);ctrl(0x61, case & 1);ctrl(0x62, (0, 1, 2, 7, 8, 16, 255)[case % 7])
        ctrl(0x63, case & 1);ctrl(0x70, case % 3 + 1);ctrl(0x71, case % 3 + 1);ctrl(0x72, case & 1)
        ctrl(0xEB, case % 5);ctrl(0x181, case);byte(0x03003D00, case)
        both(args, struct.pack("<HH", 0x181, 0x61) + rng.randbytes(12))
    def prepare(row, case):
        if selection_scene:
            ctrl(0x180,case%48);ctrl(0x182,case%5);ctrl(0x183,0);both(args,bytes([case%5]))
            if row["name"]=="ScriptCallbackRemoveMap10_5SelectionText":
                for i in range(4):
                    created=allocate_task(memory);equal(created,call("AllocateTaskContext"),"selection text removal fixture")
                    both(0x030037A0+i*4,struct.pack("<I",created))
            if row["address"] in (0x080C3A88,0x080C3B20):
                fresh();heap_init(memory);call("InitializeHeap")
                equal(initialize_map(memory,10,10),True,"selection modal setup");call("InitializeMapDescriptor",10,10)
                slots=struct.unpack("<I",native_bytes(0x03002D98,4))[0]
                both(slots,rng.randbytes(16*104));both(0x02002EB0,rng.randbytes(624))
                byte(0x03001F60,case&1);byte(0x03001F90,(case>>1)&1);byte(0x03002182,128|case&127)
                for i,index in enumerate((1,0 if case&8 else 2,3,4)):
                    byte((0x03002C0C,0x03002274,0x03001F50,0x03002270)[i],index)
                    byte(slots+index*104+96,((case+i)%4)+1 if i==0 or case&(1<<(i-1)) else 0)
                ctrl(0x0D,10);ctrl(0x0E,10)
                if row["address"]==0x080C3B20:
                    consumed=C.c_uint32();equal(main(memory,0x080C3A89,0,C.byref(consumed)),1,"selection saved actor fixture")
                    equal(consumed.value,call("ScriptCallbackSaveActorsAndChangeMap10_5",0),"modal save wrapped consumed count")
                both(0x03002C60,struct.pack("<I",0x02020111));both(args,rng.randbytes(16))
        if row["name"] == "ScriptCallbackPrepareSaveGroup3PairText":ctrl(0x60, 1 + case % 109);ctrl(0x61, 1 + case % 3)
        if row["name"] == "RenderSaveGroup3NumberFromArguments":both(args, bytes([case & 255, 255, 255]))
        if row["name"] == "ScriptCallbackCaptureSaveGroup3SelectedRecord":ctrl(0x181, case % 6);ctrl(0x61, 1 + case % 8)
        if row["name"].startswith("ScriptCallback") and "SaveGroup3List" in row["name"]:
            ctrl(0x180, case % 32)
        if row["name"] == "ScriptCallbackRevealSaveGroup3ListText":ctrl(0x60, case % 16)
        if "SaveList" in row["name"]:ctrl(0x180,case%32)
        if row["name"] in {"ScriptCallbackNavigateSaveListGrid","ScriptCallbackNavigateRecordSelection","ScriptCallbackConfigureRecordSelectionWindow"}:both(args,bytes([case%5]))
        if record_menus:
            ctrl(0x180,case%30);ctrl(0x181,case%6);ctrl(0x8A,case)
            both(0x0201CC20,bytes(255 if (case+i)%4==0 else (case+i)%10 for i in range(257)))
        if row["name"] in {"ScriptCallbackRemoveSaveListSelectionText","ScriptCallbackRemoveRecordSelectionText"}:
            for i in range(5 if record_menus else 1):
                created=allocate_task(memory);equal(created,call("AllocateTaskContext"),"fixture text task allocation")
                both(0x030037A0+i*4,struct.pack("<I",created))
    def canonical_compare(row, label):
        if row["name"] != "ScriptCallbackRevealSaveGroup3ListText":
            compare(row["name"] + " " + label, ranges);return
        native_task = struct.unpack("<I", native_bytes(0x030037A0, 4))[0]
        original_task = struct.unpack("<I", bytes(cpu.mem_read(0x030037A0, 4)))[0]
        equal(native_task, original_task, "list text allocation")
        pointer = native_task + 0x10
        native_script = struct.unpack("<I", native_bytes(pointer, 4))[0]
        original_script = struct.unpack("<I", bytes(cpu.mem_read(pointer, 4)))[0]
        equal(native_script, 0x03007D00, "native list text has stable guest storage")
        equal(0x03007000 <= original_script < 0x03007F00, True, "original list text escapes its stack frame")
        native_text = native_bytes(native_script, 20).split(b"\0", 1)[0] + b"\0"
        equal(native_text, bytes(cpu.mem_read(original_script, len(native_text))), "relocated list text prefix including terminator")
        relocated_ranges = []
        for start, size in ranges:
            if start <= pointer < start + size:
                relocated_ranges.extend(((start, pointer-start), (pointer+4, start+size-pointer-4)))
            else:relocated_ranges.append((start, size))
        compare(row["name"] + " " + label + " excluding verified relocated script pointer", relocated_ranges)
    def check(row, label, opcode=False):
        if row["name"] in {"ScriptCallbackRenderSaveGroup3StoredPairs", "ScriptCallbackRenderSaveGroup3SelectedPairs"}:
            # The first empty category reads an indeterminate stack word. Set
            # that input to the native canonical zero; direct helper checks
            # separately exercise every category with arbitrary retained Z.
            cpu.mem_write(0x03007000, bytes(0xF00))
        if opcode:
            both(args, struct.pack("<I", row["address"] | 1) + native_bytes(args, 16));byte(0x03000010, 0x1C)
            equal(command(memory, 0x1C, args), 1, row["name"] + " Main opcode");call("MainScriptInvokeCallback", args, limit=3000000)
        else:
            consumed = C.c_uint32(99)
            result = main(memory, row["address"] | 1, args, C.byref(consumed))
            equal(result, 1, row["name"] + f" native result {result} " + label)
            equal(consumed.value, call(row["name"], args, limit=3000000), row["name"] + " return " + label)
        canonical_compare(row, label)
    for row in rows:
        for case in range(64):fixture(case);prepare(row, case);check(row, str(case))
        for case in range(4):fixture(case);prepare(row, case);check(row, "Main argument cursor", opcode=True)
        fixture(0)
        if row["address"] in task_addresses:
            prepare(row, 0);both(0x02016000, rng.randbytes(52));both(0x02016010, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1));report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02016000, 0xF4, report), 1, "reviewed group3 F4 target")
            equal(report[3], call("TaskCommandF4", 0x02016000, limit=3000000), "group3 F4 continuation")
            canonical_compare(row, "F4")
        else:
            equal(apply(memory, row["address"] | 1), 0, "argument-dependent group3 target rejected by F4")
            compare("rejected group3 task preserves memory", ranges)
    if selection_scene:
        by_name={r["name"]:r for r in rows}
        text_rows=[by_name["ScriptCallbackConfigureMap10_5SelectionText"],by_name["ScriptCallbackConfigureMap10_5PagedText"]]
        for item in range(45):
            for language in range(5):
                count=rom[0x6E77D0+item*5+language]
                for page in range(0,count,3):
                    for row in text_rows:
                        fixture(item);prepare(row,item);ctrl(0xEB,language);ctrl(0x183,page)
                        byte(0x03003900+item%48,item);check(row,"all items, languages and defined text pages")
        wrapping=by_name["ScriptCallbackWrapMap10_5TextPage"]
        for page in range(256):
            for item in (0,1,44,255):
                fixture(page);prepare(wrapping,page);ctrl(0x183,page)
                byte(0x03003900+page%48,item);check(wrapping,"full-byte text page addition and empty selection")
        refresh=by_name["ScriptCallbackRefreshMap10_5SelectionSlots"]
        for selection in range(256):
            for z in (0xFFFFFFFF,0x600000,0x600001):
                fixture(selection);prepare(refresh,selection);ctrl(0x180,selection)
                both(base+0x20,struct.pack("<I",z));check(refresh,"signed threshold and byte row underflow")
        navigation=next(r for r in rows if r["name"]=="ScriptCallbackNavigateMap10_5Selection")
        for selection in range(256):
            for direction in range(5):
                fixture(selection);prepare(navigation,selection);ctrl(0x180,selection)
                both(args,bytes([direction]));z=(0xFFFFFFFF,0x5FFFFF,0x600000,0x600001)[selection%4]
                both(base+0x20,struct.pack("<I",z));check(navigation,"byte cursors, scroll thresholds and signed row start")
        render_row=by_name["ScriptCallbackConfigureRenderSlotsFromRasterE5"]
        for selector in range(256):
            fixture(selector);prepare(render_row,selector);ctrl(0xE5,selector)
            check(render_row,"full-byte panel selector including linked slot and retained actor")
        for keys_row in (by_name["ScriptCallbackChangeMap10_5FlagsFromKeys"],by_name["ScriptCallbackMarkSaveRecordsFromKeys"],by_name["ScriptCallbackFillSaveRecordsFromKeys"]):
            for keys in range(1024):
                for enabled in (False,True):
                    fixture(keys);prepare(keys_row,keys);both(0x03001F64,struct.pack("<H",1023));both(0x03000820,struct.pack("<H",keys))
                    both(0x0201CC20,rng.randbytes(257))
                    value=native_bytes(0x03002180,1)[0];byte(0x03002180,(value|4) if enabled else (value&~4))
                    check(keys_row,"all held keys, disabled flag and L/R/up/down priority")
        print("All 14 shared map-10/5 selection functions and three complete dependency families passed.")
        return
    if record_menus:
        navigation=next(r for r in rows if r["name"]=="ScriptCallbackNavigateRecordSelection")
        for selection in range(30):
            for row in (0,1,4,5):
                for direction in range(3):
                    fixture(selection);prepare(navigation,selection);ctrl(0x180,selection);ctrl(0x181,row);both(args,bytes([direction]))
                    check(navigation,"record navigation and wrap at both ends")
        window=next(r for r in rows if r["name"]=="ScriptCallbackAdvanceRecordSelectionWindow")
        for high in range(256):
            for low in (0,1,255):
                fixture(high);both(0x03002C70,struct.pack("<H",high*256+low));check(window,"scanline-window high-byte threshold and halfword wrap")
        print("All 18 record-selection functions and typed Main/task contracts passed.")
        return
    mask_rows = [r for r in rows if r["name"] in {"ScriptCallbackSelectNextSaveGroup3MaskBit", "ScriptCallbackSelectPreviousSaveGroup3MaskBit", "ScriptCallbackRecordSaveGroup3SelectedMaskBit"}]
    for row in mask_rows:
        for cursor in range(256):
            for mask in (0, 1, 2, 128, 255):
                fixture(cursor);ctrl(0x70, cursor);byte(0x03003D00, mask);check(row, "all byte cursors and mask edges")
    completed = next(r for r in rows if r["name"] == "ScriptCallbackUpdateSaveGroup3CompletedFieldMask")
    for case in range(128):
        fixture(case)
        for category in range(30):byte(fields + category * 4, 255 if case >> (category % 6) & 1 else 0)
        if case & 64:byte(fields + 25 * 4, 0)
        byte(0x03003D00, case & 63);check(completed, "completion groups, skipped category 25 and retained known bits")
    capture = next(r for r in rows if r["name"] == "ScriptCallbackCaptureSaveGroup3SelectedRecord")
    for record in (0, 5, 6, 18, 50, 255):
        fixture(record);ctrl(0x181, record);ctrl(0x61, 1);check(capture, "record overlaps selected indices and constant catalog data")
    records = next(r for r in rows if r["name"] == "ScriptCallbackRenderSaveGroup3Records")
    for style in range(9):
        fixture(style)
        for index in range(6):byte(0x03003730 + index*16, style)
        check(records, "all eight styles and empty record backgrounds")
    equal(rom[0x58DADC], 0, "empty-category default OBJ frame has no parts")
    navigation=next(r for r in rows if r["name"]=="ScriptCallbackNavigateSaveListGrid")
    for selection in range(256):
        for direction in range(5):
            for enabled in (False,True):
                fixture(selection);ctrl(0x180,selection);both(args,bytes([direction]));value=native_bytes(0x0300219F,1)[0]
                byte(0x0300219F,(value|32) if enabled else (value&~32));check(navigation,"all byte selections, grid directions and flag 253")
    print(f"All 67 functions across seven SaveGroup3 families accounted for: {len(rows)} Main callbacks and 16 helpers.")

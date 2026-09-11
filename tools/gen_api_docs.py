#!/usr/bin/env python3
"""
Generate the Thymio 3 MicroPython API reference from the C sources.

The generator does not trust the names written in the `///` comments: the public
name of every method is read from the `..._locals_dict_table` arrays, which are
what MicroPython actually exposes to the user. A method is resolved as:

    { MP_ROM_QSTR(MP_QSTR_get_acc), MP_ROM_PTR(&imu_get_acceleration_obj) }
                       |                            |
                  public name                  function object
                                                    |
    MP_DEFINE_CONST_FUN_OBJ_1(imu_get_acceleration_obj, imu_get_acceleration)
                                                            |
                                                     C function, whose
                                                  preceding `///` block is
                                                     the documentation

Classes are resolved the same way, from the module globals table in modthymio.c:

    { MP_ROM_QSTR(MP_QSTR_IMU), MP_ROM_PTR(&thymio_imu_type) }

Usage:
    python gen_api_docs.py [SOURCE_DIR] [-o OUTPUT_DIR] [--module NAME] [--pdf FILE]

Example:
    python gen_api_docs.py ports/esp32 -o site
"""

from __future__ import annotations

import argparse
import datetime as _dt
import html
import json
import os
import re
import sys
import textwrap
from dataclasses import dataclass, field

# --------------------------------------------------------------------------
# Regular expressions
# --------------------------------------------------------------------------

# MP_DEFINE_CONST_FUN_OBJ_<kind>(obj_name, [numbers,] c_function)
RE_FUN_OBJ = re.compile(
    r"MP_DEFINE_CONST_FUN_OBJ_(\w+)\s*\(\s*([A-Za-z_]\w*)\s*,\s*([^;]*?)\)\s*;",
    re.S,
)

# STATIC const mp_rom_map_elem_t <name>[] = { ... };
RE_MAP_TABLE = re.compile(
    r"(?:static|STATIC)?\s*const\s+mp_rom_map_elem_t\s+(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;",
    re.S,
)

# One entry of a map table: { MP_ROM_QSTR(MP_QSTR_x), <value> }
RE_MAP_ENTRY = re.compile(
    r"\{\s*MP_ROM_QSTR\s*\(\s*MP_QSTR_(\w+)\s*\)\s*,\s*(.*?)\s*\}\s*,?",
    re.S,
)

# MP_DEFINE_CONST_DICT(dict_name, table_name);
RE_CONST_DICT = re.compile(
    r"MP_DEFINE_CONST_DICT\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*;"
)

# MP_DEFINE_CONST_OBJ_TYPE(type_var, MP_QSTR_Name, flags, key, value, ...);
RE_OBJ_TYPE = re.compile(
    r"MP_DEFINE_CONST_OBJ_TYPE\s*\(\s*(\w+)\s*,\s*MP_QSTR_(\w+)\s*,(.*?)\)\s*;",
    re.S,
)

# Legacy type definition: const mp_obj_type_t x_type = { ... };
RE_LEGACY_TYPE = re.compile(
    r"const\s+mp_obj_type_t\s+(\w+)\s*=\s*\{(.*?)\}\s*;",
    re.S,
)

# Definition of a C function: return type, name, parameters, opening brace.
RE_C_FUNC_DEF = re.compile(
    r"^[A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\(([^;{]*?)\)\s*\{?\s*$"
)

# `\method name(args)` / `\classmethod` / `\constructor(args)` doc tags.
RE_DOC_TAG = re.compile(r"\\(\w+)")
RE_TAG_CALL = re.compile(r"\\(?:method|classmethod|constructor|function)\s*([\w\.]*)\s*(\((.*?)\))?")

# Non fatal problems found while parsing, reported at the end of the build.
WARNINGS: list[str] = []

# --------------------------------------------------------------------------
# Data model
# --------------------------------------------------------------------------


@dataclass
class FunObj:
    """A MicroPython callable object defined with MP_DEFINE_CONST_FUN_OBJ_*."""

    obj_name: str
    c_func: str
    kind: str
    n_min: int | None = None
    n_max: int | None = None
    source: str = ""


@dataclass
class DocBlock:
    """A run of `///` (or `//`) comment lines and the symbol it documents."""

    lines: list[str]
    anchor: str | None
    tags: set[str] = field(default_factory=set)
    source: str = ""
    lineno: int = 0


@dataclass
class Entry:
    """One exposed name: a method, a module function or a constant."""

    name: str
    signature: str
    description: list[dict]
    kind: str  # "constructor" | "method" | "function" | "constant"
    c_func: str = ""
    value: str = ""
    undocumented: bool = False


@dataclass
class ApiClass:
    name: str
    type_var: str
    qstr: str
    source: str
    description: list[dict] = field(default_factory=list)
    entries: list[Entry] = field(default_factory=list)


# --------------------------------------------------------------------------
# Parsing helpers
# --------------------------------------------------------------------------


def strip_comment_marker(line: str) -> str | None:
    """Return the text of a `///` documentation line, or None.

    The indentation that follows the marker is preserved, so that Python code
    inside an `\\example` block keeps its meaning.
    """
    stripped = line.lstrip()
    if stripped.startswith("///"):
        text = stripped[3:]
        return text[1:] if text.startswith(" ") else text
    return None


def strip_plain_comment(line: str) -> str | None:
    """Return the text of a plain `//` comment line, or None."""
    stripped = line.lstrip()
    if stripped.startswith("//") and not stripped.startswith("///"):
        text = stripped[2:]
        return text[1:] if text.startswith(" ") else text
    return None


def find_anchor(lines: list[str], start: int) -> tuple[str | None, int]:
    """Find the symbol documented by a comment block ending before `start`."""
    i = start
    while i < len(lines):
        text = lines[i].strip()
        if not text or text.startswith("//"):
            i += 1
            continue
        # A doc block may sit directly on top of the MP_DEFINE_CONST_FUN_OBJ line.
        m = re.match(r"(?:static|STATIC)?\s*MP_DEFINE_CONST_FUN_OBJ_\w+\s*\(\s*(\w+)", text)
        if m:
            return m.group(1), i
        # Otherwise it documents the next C function definition.
        candidate = text
        if not candidate.endswith("{"):
            # Definitions split over several lines: glue up to the opening brace.
            j = i
            while j < len(lines) and "{" not in lines[j] and ";" not in lines[j] and j - i < 6:
                j += 1
                candidate += " " + lines[j].strip() if j < len(lines) else ""
        candidate = re.sub(r"\s+", " ", candidate).strip()
        m = RE_C_FUNC_DEF.match(candidate.rstrip("{").strip() + " {")
        if m and m.group(1) not in ("if", "for", "while", "switch", "return"):
            return m.group(1), i
        return None, i
    return None, i


def parse_doc_blocks(text: str, filename: str, allow_plain_comments: bool) -> list[DocBlock]:
    """Extract every documentation block of a file with the symbol it precedes."""
    lines = text.splitlines()
    blocks: list[DocBlock] = []
    current: list[str] = []
    current_start = 0
    plain: list[str] = []
    plain_start = 0

    for idx, line in enumerate(lines):
        doc = strip_comment_marker(line)
        if doc is not None:
            if not current:
                # A plain `//` description directly above a `///` block belongs
                # to the same documentation: keep it as the first paragraph.
                if plain:
                    current = list(plain)
                    current_start = plain_start
                    plain = []
                else:
                    current_start = idx
            current.append(doc)
            plain = []
            continue

        if current:
            anchor, _ = find_anchor(lines, idx)
            blocks.append(
                DocBlock(
                    lines=current,
                    anchor=anchor,
                    tags={m.group(1) for m in RE_DOC_TAG.finditer("\n".join(current))},
                    source=filename,
                    lineno=current_start + 1,
                )
            )
            current = []

        if allow_plain_comments:
            simple = strip_plain_comment(line)
            if simple is not None:
                if not plain:
                    plain_start = idx
                plain.append(simple)
                continue
            if plain:
                anchor, _ = find_anchor(lines, idx)
                if anchor:
                    blocks.append(
                        DocBlock(
                            lines=plain,
                            anchor=anchor,
                            tags=set(),
                            source=filename,
                            lineno=plain_start + 1,
                        )
                    )
                plain = []

    if current:
        blocks.append(
            DocBlock(
                lines=current,
                anchor=None,
                tags={m.group(1) for m in RE_DOC_TAG.finditer("\n".join(current))},
                source=filename,
                lineno=current_start + 1,
            )
        )
    return blocks


def parse_fun_objs(text: str, filename: str) -> dict[str, FunObj]:
    """Map every function object name to the C function it wraps."""
    result: dict[str, FunObj] = {}
    for match in RE_FUN_OBJ.finditer(text):
        kind = match.group(1)
        obj_name = match.group(2)
        rest = [part.strip() for part in match.group(3).split(",") if part.strip()]
        if not rest:
            continue
        c_func = rest[-1]
        numbers = [int(part) for part in rest[:-1] if part.lstrip("-").isdigit()]

        n_min = n_max = None
        if kind in ("0", "1", "2", "3"):
            n_min = n_max = int(kind)
        elif kind == "VAR" and numbers:
            n_min, n_max = numbers[0], None
        elif kind == "VAR_BETWEEN" and len(numbers) >= 2:
            n_min, n_max = numbers[0], numbers[1]
        elif kind == "KW" and numbers:
            n_min, n_max = numbers[0], None

        result[obj_name] = FunObj(obj_name, c_func, kind, n_min, n_max, filename)
    return result


def parse_map_tables(text: str) -> dict[str, list[tuple[str, str]]]:
    """Map every mp_rom_map_elem_t table name to its (public name, value) pairs."""
    tables: dict[str, list[tuple[str, str]]] = {}
    for match in RE_MAP_TABLE.finditer(text):
        name = match.group(1)
        body = match.group(2)
        entries = []
        for entry in RE_MAP_ENTRY.finditer(body):
            entries.append((entry.group(1), re.sub(r"\s+", " ", entry.group(2)).strip()))
        tables[name] = entries
    return tables


def parse_types(text: str, filename: str) -> dict[str, dict[str, str]]:
    """Map every mp_obj_type_t variable to its qstr name, make_new and locals_dict."""
    types: dict[str, dict[str, str]] = {}

    for match in RE_OBJ_TYPE.finditer(text):
        type_var, qstr, body = match.group(1), match.group(2), match.group(3)
        parts = [part.strip() for part in body.split(",") if part.strip()]
        info = {"qstr": qstr, "source": filename, "make_new": "", "locals_dict": ""}
        for key, value in zip(parts, parts[1:]):
            if key == "make_new":
                info["make_new"] = value
            elif key == "locals_dict":
                info["locals_dict"] = value.lstrip("&")
        types[type_var] = info

    for match in RE_LEGACY_TYPE.finditer(text):
        type_var, body = match.group(1), match.group(2)
        info = {"qstr": "", "source": filename, "make_new": "", "locals_dict": ""}
        qstr = re.search(r"\.name\s*=\s*MP_QSTR_(\w+)", body)
        if qstr:
            info["qstr"] = qstr.group(1)
        make_new = re.search(r"\.make_new\s*=\s*(\w+)", body)
        if make_new:
            info["make_new"] = make_new.group(1)
        locals_dict = re.search(r"\.locals_dict\s*=\s*\(?[^&]*&\s*(\w+)", body)
        if locals_dict:
            info["locals_dict"] = locals_dict.group(1)
        types.setdefault(type_var, info)

    return types


def parse_c_params(text: str, c_func: str) -> list[str] | None:
    """Return the parameter names of a C function definition, if it is found."""
    pattern = re.compile(
        r"\b" + re.escape(c_func) + r"\s*\(([^;{]*?)\)\s*\{", re.S
    )
    match = pattern.search(text)
    if not match:
        return None
    raw = match.group(1).strip()
    if not raw or raw == "void":
        return []
    params = []
    for part in raw.split(","):
        tokens = re.sub(r"[\*\[\]]", " ", part).split()
        if not tokens:
            continue
        params.append(tokens[-1])
    return params


# --------------------------------------------------------------------------
# Signature and description building
# --------------------------------------------------------------------------


def pretty_param(name: str) -> str:
    """Turn a C parameter name into a Python-looking one."""
    name = name.strip()
    if name.endswith("_in"):
        name = name[:-3]
    return name


def sanitize_args(raw: str) -> str | None:
    """Turn the argument list of a `\\method` tag into valid Python parameters.

    `[value red, value green, value blue]` becomes `red, green, blue`.
    Returns None when the text cannot be read as a parameter list.
    """
    names = []
    for item in raw.split(","):
        identifiers = re.findall(r"[A-Za-z_]\w*", item)
        if not identifiers:
            return None
        names.append(identifiers[-1])
    return ", ".join(names)


def build_signature(
    public_name: str,
    fun: FunObj,
    sources: dict[str, str],
    is_method: bool,
    doc_args: str | None,
) -> str:
    """Build the Python signature of an exposed callable."""
    if doc_args is not None and doc_args.strip():
        cleaned = sanitize_args(doc_args)
        if cleaned is not None:
            expected = None
            if fun.n_min is not None and fun.n_min == fun.n_max:
                expected = fun.n_min - (1 if is_method else 0)
            given = len([n for n in cleaned.split(",") if n.strip()])
            if expected is not None and expected != given:
                WARNINGS.append(
                    f"{public_name}: the comment declares {given} arguments, "
                    f"the C binding takes {expected}"
                )
            return f"{public_name}({cleaned})"

    text = sources.get(fun.source, "")
    params = parse_c_params(text, fun.c_func)

    variadic = fun.kind in ("VAR", "VAR_BETWEEN", "KW")
    if params is not None and not variadic:
        names = [pretty_param(p) for p in params]
        if is_method and names:
            names = names[1:]
        return f"{public_name}({', '.join(names)})"

    # Variadic form: the parameter names are hidden behind `args`, so the arity
    # of the macro is the only reliable information.
    offset = 1 if is_method else 0
    n_min = (fun.n_min or offset) - offset
    n_max = None if fun.n_max is None else fun.n_max - offset
    names = [f"arg{i + 1}" for i in range(n_min)]
    if n_max is not None and n_max > n_min:
        names += [f"arg{i + 1}=None" for i in range(n_min, n_max)]
    elif n_max is None:
        names.append("*args")
    if fun.kind == "KW":
        names.append("**kwargs")
    return f"{public_name}({', '.join(names)})"


def clean_description(lines: list[str]) -> list[dict]:
    """Turn raw comment lines into ordered blocks of prose and code examples.

    Prose lines are wrapped into paragraphs, separated by empty comment lines.
    A line starting with `\\example` opens a verbatim code block that runs until
    `\\endexample`, until the next `\\tag`, or until the end of the comment.
    """
    blocks: list[dict] = []
    buffer: list[str] = []
    code: list[str] | None = None
    caption = ""

    def flush_text() -> None:
        nonlocal buffer
        if buffer:
            blocks.append({"type": "text", "text": " ".join(buffer)})
            buffer = []

    def flush_code() -> None:
        nonlocal code, caption
        if code is not None:
            snippet = "\n".join(code).strip("\n")
            snippet = textwrap.dedent(snippet).rstrip()
            if snippet:
                blocks.append({"type": "code", "caption": caption, "code": snippet})
        code = None
        caption = ""

    for line in lines:
        text = line.strip()
        tag = RE_DOC_TAG.match(text)

        if tag and tag.group(1) == "example":
            flush_text()
            flush_code()
            code = []
            caption = text[len(tag.group(0)):].strip()
            continue

        if code is not None:
            if tag and tag.group(1) in ("endexample", "endcode"):
                flush_code()
                continue
            if tag:
                flush_code()
            else:
                code.append(line.rstrip())
                continue

        if tag:
            name = tag.group(1)
            rest = text[len(tag.group(0)):].strip()

            if name == "param":
                flush_text()
                param = re.match(r"([\w\[\]]+)\s*[-:]?\s*(.*)", rest)
                if param:
                    blocks.append(
                        {
                            "type": "param",
                            "name": param.group(1).strip("[]"),
                            "text": param.group(2).strip(),
                        }
                    )
                continue

            if name in ("method", "classmethod", "constructor", "function"):
                # Drop the declaration itself: the real name and signature are
                # read from the locals_dict table, not from the comment.
                rest = re.sub(r"^[\w\.]*\s*(\(.*\))?\s*", "", rest)
            rest = re.sub(r"^[-:]\s*", "", rest)

            # A `\class IMU - IMU object` line is a title, not a paragraph.
            text = rest
            if not text or len(text.split()) <= 4:
                continue

        if not text:
            flush_text()
            continue
        buffer.append(text)

    flush_text()
    flush_code()
    return blocks


def text_paragraphs(blocks: list[dict]) -> list[str]:
    """Keep only the prose of a block list."""
    return [b["text"] for b in blocks if b["type"] == "text"]


def doc_args_of(block: DocBlock | None) -> str | None:
    """Return the argument list written in a `\\method name(args)` tag, if any."""
    if not block:
        return None
    for line in block.lines:
        # A line may carry two tags, as in `\classmethod \constructor(id)`.
        for match in RE_TAG_CALL.finditer(line.strip()):
            if match.group(3):
                return match.group(3)
    return None


# --------------------------------------------------------------------------
# Model building
# --------------------------------------------------------------------------


def read_api_version(source_dir: str, header: str = "modthymio.h") -> str | None:
    """Read THYMIO_API_MAJOR/MINOR_VERSION from the module header."""
    for root, _, files in os.walk(source_dir):
        if header not in files:
            continue
        with open(os.path.join(root, header), "r", encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
        major = re.search(r"#define\s+\w*API_MAJOR_VERSION\s+(\d+)", text)
        minor = re.search(r"#define\s+\w*API_MINOR_VERSION\s+(\d+)", text)
        if major and minor:
            return f"{major.group(1)}.{minor.group(1)}"
    return None


def collect_sources(source_dir: str, prefixes: tuple[str, ...], extra: tuple[str, ...]):
    """Find the source files that make up the API."""
    found = []
    for root, _, files in os.walk(source_dir):
        for name in sorted(files):
            if not name.endswith((".c", ".h", ".cpp")):
                continue
            if name in extra or any(name.startswith(p) for p in prefixes):
                found.append(os.path.join(root, name))
    return found


def build_api(source_dir: str, module_file: str, module_name: str, prefixes: tuple[str, ...]):
    """Parse every source file and assemble the API model."""
    paths = collect_sources(source_dir, prefixes, (module_file,))
    if not paths:
        raise SystemExit(f"No source file matching {prefixes} or {module_file} under {source_dir!r}")

    sources: dict[str, str] = {}
    fun_objs: dict[str, FunObj] = {}
    tables: dict[str, list[tuple[str, str]]] = {}
    dicts: dict[str, str] = {}
    types: dict[str, dict[str, str]] = {}
    docs_by_symbol: dict[str, DocBlock] = {}
    class_docs: dict[str, DocBlock] = {}
    module_path = None

    for path in paths:
        filename = os.path.basename(path)
        with open(path, "r", encoding="utf-8", errors="ignore") as handle:
            text = handle.read()
        sources[filename] = text
        if filename == module_file:
            module_path = filename

        fun_objs.update(parse_fun_objs(text, filename))
        tables.update(parse_map_tables(text))
        for match in RE_CONST_DICT.finditer(text):
            dicts[match.group(1)] = match.group(2)
        types.update(parse_types(text, filename))

        for block in parse_doc_blocks(text, filename, allow_plain_comments=True):
            if block.tags & {"moduleref", "class"} or (block.anchor is None and block.lines):
                class_docs.setdefault(filename, block)
            if block.anchor:
                docs_by_symbol.setdefault(f"{filename}:{block.anchor}", block)
                docs_by_symbol.setdefault(block.anchor, block)

    def doc_for(filename: str, symbol: str) -> DocBlock | None:
        return docs_by_symbol.get(f"{filename}:{symbol}") or docs_by_symbol.get(symbol)

    def entries_of_table(table_name: str, is_method: bool) -> list[Entry]:
        result: list[Entry] = []
        for public_name, value in tables.get(table_name, []):
            if public_name in ("__name__",):
                continue
            ref = re.match(r"MP_ROM_PTR\s*\(\s*&\s*(\w+)\s*\)", value)
            if ref and ref.group(1) in fun_objs:
                fun = fun_objs[ref.group(1)]
                block = doc_for(fun.source, fun.c_func) or doc_for(fun.source, fun.obj_name)
                description = clean_description(block.lines) if block else []
                result.append(
                    Entry(
                        name=public_name,
                        signature=build_signature(
                            public_name, fun, sources, is_method, doc_args_of(block)
                        ),
                        description=description,
                        kind="method" if is_method else "function",
                        c_func=fun.c_func,
                        undocumented=not description,
                    )
                )
            elif ref and ref.group(1) in types:
                continue  # nested type, handled as a class
            else:
                literal = re.sub(r"^MP_ROM_(INT|QSTR|PTR)\s*\((.*)\)$", r"\2", value).strip()
                result.append(
                    Entry(
                        name=public_name,
                        signature=public_name,
                        description=[],
                        kind="constant",
                        value=literal,
                        undocumented=True,
                    )
                )
        return result

    # Classes and module functions, in the order of the module globals table.
    if module_path is None:
        raise SystemExit(f"{module_file} not found under {source_dir!r}")

    module_table = None
    for name in tables:
        if name.startswith(f"{module_name}_module_globals") or name.endswith("module_globals_table"):
            module_table = name
            break
    if module_table is None:
        raise SystemExit(f"No module globals table found in {module_file}")

    classes: list[ApiClass] = []
    module_functions: list[Entry] = []

    for public_name, value in tables[module_table]:
        if public_name == "__name__":
            continue
        ref = re.match(r"MP_ROM_PTR\s*\(\s*&\s*(\w+)\s*\)", value)
        if not ref:
            literal = re.sub(r"^MP_ROM_(INT|QSTR)\s*\((.*)\)$", r"\2", value).strip()
            module_functions.append(
                Entry(public_name, public_name, [], "constant", value=literal, undocumented=True)
            )
            continue

        target = ref.group(1)
        if target in types:
            info = types[target]
            api_class = ApiClass(
                name=public_name,
                type_var=target,
                qstr=info.get("qstr", public_name),
                source=info.get("source", ""),
            )
            block = class_docs.get(api_class.source)
            api_class.description = clean_description(block.lines) if block else []

            make_new = info.get("make_new")
            if make_new:
                ctor_block = doc_for(api_class.source, make_new)
                ctor_params = parse_c_params(sources.get(api_class.source, ""), make_new)
                args = doc_args_of(ctor_block)
                if args is None and ctor_params and len(ctor_params) >= 4:
                    args = ""  # standard (type, n_args, n_kw, args) signature
                api_class.entries.append(
                    Entry(
                        name=public_name,
                        signature=f"{public_name}({args or ''})",
                        description=clean_description(ctor_block.lines) if ctor_block else [],
                        kind="constructor",
                        c_func=make_new,
                        undocumented=not ctor_block,
                    )
                )

            table_name = dicts.get(info.get("locals_dict", ""), "")
            api_class.entries.extend(entries_of_table(table_name, is_method=True))
            classes.append(api_class)

        elif target in fun_objs:
            fun = fun_objs[target]
            block = doc_for(fun.source, fun.c_func) or doc_for(fun.source, fun.obj_name)
            description = clean_description(block.lines) if block else []
            module_functions.append(
                Entry(
                    name=public_name,
                    signature=build_signature(public_name, fun, sources, False, doc_args_of(block)),
                    description=description,
                    kind="function",
                    c_func=fun.c_func,
                    undocumented=not description,
                )
            )

    return {
        "module": module_name,
        "version": read_api_version(source_dir),
        "functions": module_functions,
        "classes": classes,
        "files": sorted(sources),
    }


# --------------------------------------------------------------------------
# Rendering
# --------------------------------------------------------------------------

CSS = """
:root {
  --paper: #fbfaf7;
  --card: #ffffff;
  --ink: #16191d;
  --muted: #5f676f;
  --rule: #e2e0d8;
  --accent: #2f5d50;
  --accent-soft: #e8efec;
  --warn: #8a5a12;
  --warn-soft: #fbf1dd;
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; scroll-padding-top: 1.5rem; }
body {
  margin: 0;
  background: var(--paper);
  color: var(--ink);
  font-family: "IBM Plex Sans", system-ui, -apple-system, "Segoe UI", sans-serif;
  font-size: 16px;
  line-height: 1.6;
}
code, .sig, .mono {
  font-family: "IBM Plex Mono", ui-monospace, SFMono-Regular, Menlo, monospace;
}
a { color: var(--accent); }

.masthead {
  border-bottom: 1px solid var(--rule);
  padding: 2.5rem 1.5rem 1.75rem;
  background: var(--card);
}
.masthead-inner { max-width: 1180px; margin: 0 auto; }
.masthead h1 {
  margin: 0;
  font-size: clamp(1.9rem, 4vw, 2.6rem);
  font-weight: 600;
  letter-spacing: -0.02em;
}
.masthead h1 .mono { color: var(--accent); font-weight: 500; }
.masthead p { margin: 0.5rem 0 0; color: var(--muted); max-width: 62ch; }
.meta { margin-top: 1rem; color: var(--muted); font-size: 0.85rem; }

.layout {
  max-width: 1180px;
  margin: 0 auto;
  padding: 2rem 1.5rem 5rem;
  display: grid;
  grid-template-columns: 250px minmax(0, 1fr);
  gap: 3rem;
  align-items: start;
}
.sidebar { position: sticky; top: 1.5rem; }
#filter {
  width: 100%;
  padding: 0.6rem 0.75rem;
  font: inherit;
  font-size: 0.9rem;
  color: var(--ink);
  background: var(--card);
  border: 1px solid var(--rule);
  border-radius: 6px;
}
#filter:focus { outline: 2px solid var(--accent); outline-offset: 1px; }
.nav-count { font-size: 0.8rem; color: var(--muted); margin: 0.6rem 0 1rem; }
.sidebar nav ul { list-style: none; margin: 0; padding: 0; }
.sidebar nav > ul > li { margin-bottom: 0.35rem; }
.sidebar nav a {
  display: block;
  padding: 0.2rem 0;
  text-decoration: none;
  color: var(--ink);
  font-size: 0.92rem;
}
.sidebar nav a:hover { color: var(--accent); text-decoration: underline; }

section.api { margin-bottom: 3.5rem; scroll-margin-top: 1.5rem; }
section.api > h2 {
  margin: 0 0 0.35rem;
  font-size: 1.5rem;
  font-weight: 600;
  letter-spacing: -0.01em;
}
section.api > h2 .from { font-size: 0.8rem; color: var(--muted); font-weight: 400; margin-left: 0.6rem; }
.class-desc { margin: 0 0 1.5rem; color: var(--muted); max-width: 72ch; }

.entry {
  border-top: 1px solid var(--rule);
  padding: 1.1rem 0 1.2rem;
  scroll-margin-top: 1.5rem;
}
.entry:last-child { border-bottom: 1px solid var(--rule); }
.sig {
  font-size: 0.98rem;
  font-weight: 500;
  color: var(--ink);
  display: block;
  word-break: break-word;
}
.sig .owner { color: var(--muted); }
.sig .name { color: var(--accent); }
.entry p { margin: 0.55rem 0 0; max-width: 72ch; }
.entry p:first-of-type { margin-top: 0.6rem; }
.note {
  display: inline-block;
  margin-top: 0.6rem;
  padding: 0.15rem 0.5rem;
  font-size: 0.78rem;
  color: var(--warn);
  background: var(--warn-soft);
  border-radius: 4px;
}
.const-value { color: var(--muted); }
.empty { color: var(--muted); font-style: italic; }

.params {
  margin: 0.8rem 0 0;
  max-width: 72ch;
  display: grid;
  grid-template-columns: minmax(4rem, max-content) 1fr;
  gap: 0.25rem 1rem;
}
.params dt { font-family: "IBM Plex Mono", ui-monospace, monospace; font-size: 0.87rem; }
.params dd { margin: 0; color: var(--muted); }

.example {
  position: relative;
  margin: 0.9rem 0 0;
  max-width: 72ch;
  background: #f4f2ec;
  border: 1px solid var(--rule);
  border-radius: 6px;
}
.example figcaption {
  padding: 0.5rem 0.9rem 0;
  font-size: 0.85rem;
  color: var(--muted);
}
.example pre {
  margin: 0;
  padding: 0.85rem 0.9rem;
  overflow-x: auto;
}
.example code {
  font-size: 0.87rem;
  line-height: 1.55;
  color: var(--ink);
  white-space: pre;
}
.example .copy {
  position: absolute;
  top: 0.45rem;
  right: 0.45rem;
  padding: 0.15rem 0.5rem;
  font: inherit;
  font-size: 0.75rem;
  color: var(--muted);
  background: var(--card);
  border: 1px solid var(--rule);
  border-radius: 4px;
  cursor: pointer;
  opacity: 0;
}
.example:hover .copy, .example .copy:focus { opacity: 1; }
.example .copy:hover { color: var(--accent); border-color: var(--accent); }
.class-desc .example { margin-bottom: 1.5rem; }

.pdf-link {
  display: inline-block;
  margin-top: 0.9rem;
  padding: 0.3rem 0.7rem;
  font-size: 0.85rem;
  text-decoration: none;
  color: var(--accent);
  border: 1px solid var(--accent);
  border-radius: 5px;
}
.pdf-link:hover { background: var(--accent-soft); }

@page {
  size: A4;
  margin: 18mm 16mm 16mm;
  @bottom-right {
    content: counter(page) " / " counter(pages);
    font-family: "IBM Plex Sans", sans-serif;
    font-size: 8pt;
    color: #5f676f;
  }
  @bottom-left {
    content: string(doctitle);
    font-family: "IBM Plex Sans", sans-serif;
    font-size: 8pt;
    color: #5f676f;
  }
}

@media print {
  body { font-size: 10pt; background: #ffffff; }
  .masthead { padding: 0 0 1rem; border: none; background: none; }
  .masthead h1 { font-size: 20pt; string-set: doctitle content(); }
  .sidebar, .pdf-link, .copy { display: none; }
  .layout { display: block; max-width: none; padding: 0; }
  section.api { page-break-before: always; margin-bottom: 0; }
  section.api:first-of-type { page-break-before: avoid; }
  section.api > h2 { page-break-after: avoid; font-size: 14pt; }
  .entry { page-break-inside: avoid; }
  .example { background: #f4f2ec; }
  a { color: inherit; text-decoration: none; }
}

@media (max-width: 860px) {
  .layout { grid-template-columns: 1fr; gap: 1.5rem; }
  .sidebar { position: static; }
}
@media (prefers-reduced-motion: reduce) {
  html { scroll-behavior: auto; }
}
"""

JS = """
const filter = document.getElementById('filter');
const sections = Array.from(document.querySelectorAll('section.api'));
const navItems = Array.from(document.querySelectorAll('#nav li[data-name]'));
const count = document.getElementById('count');

function apply() {
  const q = filter.value.trim().toLowerCase();
  let visible = 0;
  sections.forEach(section => {
    const title = section.dataset.name.toLowerCase();
    let hits = 0;
    section.querySelectorAll('.entry').forEach(entry => {
      const match = !q || title.includes(q) || entry.dataset.search.includes(q);
      entry.hidden = !match;
      if (match) hits++;
    });
    section.hidden = hits === 0;
    if (hits) visible += hits;
  });
  navItems.forEach(item => {
    const section = document.getElementById(item.dataset.target);
    item.hidden = !!section && section.hidden;
  });
  count.textContent = q ? visible + ' matching' : count.dataset.total;
}

document.querySelectorAll('.example .copy').forEach(button => {
  button.addEventListener('click', async () => {
    await navigator.clipboard.writeText(button.parentElement.querySelector('code').textContent);
    button.textContent = 'Copied';
    setTimeout(() => { button.textContent = 'Copy'; }, 1500);
  });
});

filter.addEventListener('input', apply);
"""


def esc(text: str) -> str:
    """Escape a string and mark quoted identifiers as inline code."""
    out = html.escape(text)
    out = re.sub(r"&quot;([A-Za-z_][\w\.]*)&quot;", r"<code>\1</code>", out)
    out = re.sub(r"`([^`]+)`", r"<code>\1</code>", out)
    return out


def slug(*parts: str) -> str:
    return "-".join(re.sub(r"[^A-Za-z0-9]+", "-", p).strip("-") for p in parts if p).lower()


def render_blocks(blocks: list[dict]) -> str:
    """Render prose, parameters and code examples in the order they were written."""
    parts = []
    params: list[dict] = []

    def flush_params() -> None:
        nonlocal params
        if params:
            rows = "".join(
                f'<dt><code>{html.escape(p["name"])}</code></dt><dd>{esc(p["text"])}</dd>'
                for p in params
            )
            parts.append(f'<dl class="params">{rows}</dl>')
            params = []

    for block in blocks:
        if block["type"] == "param":
            params.append(block)
            continue
        flush_params()
        if block["type"] == "text":
            parts.append(f"<p>{esc(block['text'])}</p>")
        else:
            caption = (
                f'<figcaption>{esc(block["caption"])}</figcaption>' if block["caption"] else ""
            )
            parts.append(
                '<figure class="example">'
                f"{caption}"
                '<button class="copy" type="button">Copy</button>'
                f'<pre><code>{html.escape(block["code"])}</code></pre>'
                "</figure>"
            )
    flush_params()
    return "".join(parts)


def render_entry(owner: str, entry: Entry, module: str) -> str:
    """Render one entry. Methods are shown bare, because they are called on an
    instance whose name is chosen by the user, not on the class."""
    anchor = slug(owner, entry.name)
    search = " ".join(
        [entry.name, entry.signature]
        + [b.get("text") or b.get("code", "") for b in entry.description]
    ).lower()

    # Only the module prefix is a real call path: `thymio.IMU()`, `thymio.turn_off_all()`.
    prefix = ""
    if entry.kind in ("constructor", "function"):
        prefix = f'<span class="owner">{html.escape(module)}.</span>'

    if entry.kind == "constant":
        sig = f'{prefix}<span class="name">{html.escape(entry.name)}</span>'
        body = f'<p class="const-value">Constant value: <code>{html.escape(entry.value)}</code></p>'
    else:
        name, _, args = entry.signature.partition("(")
        sig = f'{prefix}<span class="name">{html.escape(name)}</span>({html.escape(args[:-1])})'
        body = render_blocks(entry.description)
        if not body:
            body = '<p class="note">Not documented in the source yet.</p>'

    return (
        f'<div class="entry" id="{anchor}" data-search="{html.escape(search)}">'
        f'<code class="sig">{sig}</code>{body}</div>'
    )


def render_html(api: dict, title: str, subtitle: str, pdf_name: str = "") -> str:
    module = api["module"]
    version = api.get("version")

    nav = []
    sections = []
    total = 0

    if api["functions"]:
        anchor = slug(module, "functions")
        nav.append(
            f'<li data-name="{module} functions" data-target="{anchor}">'
            f'<a href="#{anchor}">Module functions</a></li>'
        )
        entries = "".join(render_entry(module, e, module) for e in api["functions"])
        total += len(api["functions"])
        sections.append(
            f'<section class="api" id="{anchor}" data-name="{html.escape(module)} functions">'
            f"<h2>Module functions</h2>"
            f'<p class="class-desc">Called directly on the <code>{html.escape(module)}</code> '
            f"module, without creating an object.</p>{entries}</section>"
        )

    for api_class in api["classes"]:
        anchor = slug(module, api_class.name)
        nav.append(
            f'<li data-name="{html.escape(api_class.name)}" data-target="{anchor}">'
            f'<a href="#{anchor}">{html.escape(api_class.name)}</a></li>'
        )
        entries = "".join(render_entry(api_class.name, e, module) for e in api_class.entries)
        total += len(api_class.entries)
        desc = f'<div class="class-desc">{render_blocks(api_class.description)}</div>' if api_class.description else ""
        body = entries or '<p class="empty">No public members.</p>'
        sections.append(
            f'<section class="api" id="{anchor}" data-name="{html.escape(api_class.name)}">'
            f"<h2>{html.escape(api_class.name)}"
            f'<span class="from">{html.escape(api_class.source)}</span></h2>'
            f"{desc}{body}</section>"
        )

    pdf_button = (
        f'<a class="pdf-link" href="{html.escape(pdf_name)}" download>Download the PDF version</a>'
        if pdf_name
        else ""
    )
    counts = f"{len(api['classes'])} classes, {total} entries"
    version_line = html.escape(f"API version {version}. {counts}." if version else f"{counts}.")

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=IBM+Plex+Sans:wght@400;500;600&display=swap" rel="stylesheet">
<style>{CSS}</style>
</head>
<body>
<header class="masthead">
  <div class="masthead-inner">
    <h1>{html.escape(title)}</h1>
    <p>{html.escape(subtitle)}</p>
    <p class="meta">{version_line}</p>
    {pdf_button}
  </div>
</header>
<div class="layout">
  <aside class="sidebar">
    <input id="filter" type="search" placeholder="Search a method"
           aria-label="Search a method" autocomplete="off">
    <p class="nav-count" id="count" data-total="{total} entries">{total} entries</p>
    <nav><ul id="nav">{"".join(nav)}</ul></nav>
  </aside>
  <main>{"".join(sections)}</main>
</div>
<script>{JS}</script>
</body>
</html>
"""


def entry_to_dict(entry: Entry) -> dict:
    """Serialise an entry, exposing both the plain text and the ordered blocks."""
    data = vars(entry).copy()
    data["blocks"] = entry.description
    data["description"] = text_paragraphs(entry.description)
    data["examples"] = [b["code"] for b in entry.description if b["type"] == "code"]
    return data


def api_to_json(api: dict) -> str:
    payload = {
        "module": api["module"],
        "version": api.get("version"),
        "generated": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
        "functions": [entry_to_dict(e) for e in api["functions"]],
        "classes": [
            {
                "name": c.name,
                "source": c.source,
                "type": c.type_var,
                "description": text_paragraphs(c.description),
                "blocks": c.description,
                "entries": [entry_to_dict(e) for e in c.entries],
            }
            for c in api["classes"]
        ],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False)


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Generate the Thymio 3 MicroPython API reference.")
    parser.add_argument("source_dir", nargs="?", default=".", help="folder holding the C sources")
    parser.add_argument("-o", "--output", default="site", help="output folder (default: site)")
    parser.add_argument("--module", default="thymio", help="MicroPython module name")
    parser.add_argument("--module-file", default="modthymio.c", help="file holding the module table")
    parser.add_argument("--prefix", default="thymio_", help="prefix of the class source files")
    parser.add_argument("--title", default="Thymio 3 MicroPython API")
    parser.add_argument("--subtitle", default="Reference for the thymio module of the Thymio 3 firmware.")
    parser.add_argument(
        "--pdf",
        nargs="?",
        const="thymio-api.pdf",
        metavar="FILE",
        help="also write a PDF next to the page and link it (needs weasyprint)",
    )
    parser.add_argument("--strict", action="store_true", help="exit with an error if an entry is undocumented")
    args = parser.parse_args(argv)

    api = build_api(args.source_dir, args.module_file, args.module, (args.prefix,))

    os.makedirs(args.output, exist_ok=True)

    # A bare file name is written inside the output folder, so that the page and
    # the PDF are published together and the link stays relative.
    pdf_path = ""
    pdf_name = ""
    if args.pdf:
        if os.path.dirname(args.pdf):
            pdf_path = args.pdf
            pdf_name = os.path.relpath(args.pdf, args.output)
        else:
            pdf_path = os.path.join(args.output, args.pdf)
            pdf_name = args.pdf

    page = render_html(api, args.title, args.subtitle, pdf_name)
    index = os.path.join(args.output, "index.html")
    with open(index, "w", encoding="utf-8") as handle:
        handle.write(page)
    with open(os.path.join(args.output, "api.json"), "w", encoding="utf-8") as handle:
        handle.write(api_to_json(api))

    missing = [
        f"{c.name}.{e.name}"
        for c in api["classes"]
        for e in c.entries
        if e.undocumented and e.kind != "constant"
    ] + [f"{api['module']}.{e.name}" for e in api["functions"] if e.undocumented and e.kind != "constant"]

    total = sum(len(c.entries) for c in api["classes"]) + len(api["functions"])
    print(f"Parsed {len(api['files'])} files: {len(api['classes'])} classes, {total} entries.")
    print(f"Wrote {index}")
    if missing:
        print(f"Undocumented entries ({len(missing)}): {', '.join(missing)}")
    for warning in WARNINGS:
        print(f"Warning: {warning}")

    if pdf_path:
        try:
            from weasyprint import HTML  # noqa: PLC0415
        except ImportError:
            print(
                "weasyprint is not installed, skipping the PDF "
                "(pip install weasyprint).",
                file=sys.stderr,
            )
        else:
            HTML(filename=index).write_pdf(pdf_path)
            print(f"Wrote {pdf_path}")

    if args.strict and missing:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

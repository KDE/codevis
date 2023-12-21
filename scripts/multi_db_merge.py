#!/usr/bin/env python3

import distutils.spawn
import sys
import argparse
import subprocess
import os
import sqlite3

from pathlib import Path
import json

########################
# Argument Management  #
########################
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Launch create_codebase_db, one per compile_commands.json entry')

    parser.add_argument('--compile-commands', help='The path of the compile commands file.')
    parser.add_argument('--source-path', help='the source path of the project)')

    args = parser.parse_args()
    print(args.__dict__)

    if args.compile_commands is None:
        print("--compile-commands is a required argument")
    if args.source_path is None:
        print("--source-path is a required argument")

    if (args.compile_commands is None or args.source_path is None):
        sys.exit(1)

    #######################
    # Argument Validation
    #######################
    compile_commands_path = Path(args.compile_commands)
    validate = True

    if args.compile_commands.split('/')[-1] != "compile_commands.json":
        print("Compile commands json file must be a path ponting to a compile_commands.json")
        validate = False

    if not compile_commands_path.is_file():
        print("Point the path to an existing compile commands file.")
        validate = False
    source_path = Path(args.source_path)
    if not source_path.is_dir():
        print("Point the source path to a directory containing reall")
        validate = False

    found = distutils.spawn.find_executable("codevis_create_codebase_db")
    if not found:
        print("Please set your PATH environment to a place that has codevis_create_codebase_db")
        validate = False

    if not validate:
        sys.exit()

    ############################
    # Script Logic Starts Here
    ############################

    cmd_str = f"codevis_create_codebase_db --query-system-headers"
    result = subprocess.run(cmd_str, shell=True)
    use_system_headers = "yes" if result.returncode == 1 else "no"

    generated_files = []
    resolved_path = compile_commands_path.parent.resolve()

    with open(compile_commands_path) as cc_file:
        commands = json.load(cc_file)
        print(f"Looping through {len(commands)} compilation commands")

    ############################################################################################
    # First we iterate through all the compile commands an generate the db for each object file
    ############################################################################################
    total = len(commands)
    curr = 1
    for command in commands:
        output = command["output"] + ".db"
        compile_command = command["command"]
        dict_str = '"' + str(command) + '"'
        dict_str = dict_str.replace("'", "\\\"")
        cmd_str = f"codevis_create_codebase_db --source-path {source_path} --output  {output} --compile-command {dict_str} --use-system-headers {use_system_headers} --replace --silent"
        print(f"{curr} of {total}: Generating {output}")
        curr += 1
        generated_files.append(resolved_path / output)
        result = subprocess.run(cmd_str, shell=True, cwd=resolved_path)
        if (result.returncode != 0):
           print(f"Trying to save database file {output}")
           print("Error running the script, see output")
           sys.exit()

    ############################################################################
    # Then we iterate thorugh it again to merge them back into a single db file
    ############################################################################
    database_str = ""
    for gen_file in generated_files:
        database_str += "--database " + str(resolved_path / gen_file) + " "


    ############################################################################
    # Delete the possible stray files on the disk, to re-generate them.
    ############################################################################
    if os.path.exists("single.db"):
        os.remove("single.db")
    if os.path.exists("multi.db"):
        os.remove("multi.db")

    ############################################################################
    # Generate the single database from the multiple db parts
    ############################################################################
    cmd_str = f"codevis_merge_databases {database_str} --output multi.db"
    subprocess.run(cmd_str, shell=True)
    print("Generating single db from merged databases successfully")

    ############################################################################
    # Generate the single database by re-scanning the entire codebase
    # using our multithreaded code
    ############################################################################
    cmd_str = f"codevis_create_codebase_db --source-path {source_path} --output single.db --compile-commands-json {compile_commands_path} --use-system-headers {use_system_headers} --replace --silent"
    result = subprocess.run(cmd_str, shell=True)
    if (result.returncode != 0):
        print(f"Trying to save database file {output}")
        print("Error running the script, see output")
    else:
        print("Generating db from multithreaded calls successfully")

    ##################################
    # Compare both databases
    ##################################
    multi_db_sqlite = sqlite3.connect("multi.db");
    single_db_sqlite = sqlite3.connect("single.db");

    multi_cur = multi_db_sqlite.cursor()
    single_cur = single_db_sqlite.cursor()

    queries = [
        "SELECT qualified_name FROM class_declaration",
        "SELECT qualified_name FROM field_declaration",
        "SELECT qualified_name FROM function_declaration",
        "SELECT qualified_name FROM method_declaration",
        "SELECT qualified_name FROM namespace_declaration",
        "SELECT qualified_name FROM source_component",
        "SELECT qualified_name FROM source_file",
        "SELECT qualified_name FROM source_package",
        "SELECT qualified_name FROM source_repository",
        "SELECT qualified_name FROM variable_declaration",
    ]

    has_error = False
    for query in queries:
        single_names = []
        multi_names = []
        for row in multi_cur.execute(query):
            multi_names.append(row[0])
        for row in single_cur.execute(query):
            single_names.append(row[0])

        single_names.sort()
        multi_names.sort()
        if (single_names != multi_names):
            single_names_set = set(single_names)
            multi_names_set = set(multi_names)

            not_in_multi_db = single_names_set.difference(multi_names_set)
            not_in_single_db = multi_names_set.difference(single_names_set)

            print("\n")
            print("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n")
            print(f"FAIL: {query}")
            print(f"Entities not in the single database:\n {not_in_single_db}")
            print(f"Entities not in the joined database:\n {not_in_multi_db}")
            has_error = True
        else:
            print(f"OK: {query} ")

    if has_error:
        sys.exit(1)

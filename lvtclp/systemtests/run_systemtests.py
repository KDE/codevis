#!/usr/bin/python3

import os
import sys
import subprocess
import glob
import time
import argparse
import shutil

builddir = "build"

def load_folders():
    full_list = []

    for fileitem in os.listdir():
        if fileitem == "build":
            continue

        if os.path.isdir(fileitem):
            full_list.append(fileitem)

    return full_list


def configure_project(project_build_folder, project):
    arguments = [
        "cmake",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=1",
        os.path.join("../../", project)
    ]

    subprocess.run(args=arguments, cwd=project_build_folder)


def make_project(project_build_folder, rebuild):
    if rebuild:
        subprocess.run(args=["make", "clean"], cwd=project_build_folder)
    subprocess.run(args=["make"], cwd=project_build_folder)


def extract_llvm_information(project_build_folder, project):
    ignored_item = []
    ignored_items = glob.glob(project + "/*.t.cpp")
    ignored_items.extend(glob.glob(project + "/*thirdparty*"))

    arguments = [
#       "valgrind",
#       "--tool=callgrind",
        "create_codebase_db",
        "--update"
    ]

    for ignored_item in ignored_items:
        arguments.append("--ignore")
        arguments.append(ignored_item)

    arguments.append(os.path.abspath(os.path.join(project_build_folder, project + ".db")))
    arguments.append(project_build_folder + "/compile_commands.json")

    start = time.time()
    result = subprocess.run(arguments)
    end = time.time()
    print("create_codebase_db took:", end - start, "msec");


def copy_current_database(project_build_folder, project):
    source = os.path.join(project_build_folder, project + ".db")
    if not os.path.exists(source):
        raise f"Blessing error, database for {project} does not exist"

    destination = os.path.join(project, project + ".db")

    shutil.move(source, destination)

    retvalue = subprocess.run(["git", "diff", "--exit-code"])
    if retvalue.returncode == 0:
        print("Nothing to do.")
        return

    commands = [
        ["git", "add", destination],
        ["git", "commit", "-m", f"Automatic commit with new database for {project}"],
   #     ["git", "push", "origin", "master"]
    ]

    for command in commands:
        subprocess.check_output(command)

def create_build_folder(base_build_folder, project):
    project_build_folder = os.path.join(builddir, project)

    if not os.path.exists(project_build_folder):
        os.makedirs(project_build_folder)

    return project_build_folder

def run_diffoscope(project_build_folder, project):
    arguments_orig_db = [
        "dump_database",
        os.path.join(project, project + ".db")
    ]

    arguments_new_db = [
        "dump_database",
        os.path.join(project_build_folder, project + ".db")
    ]

    orig_output = subprocess.check_output(arguments_orig_db)
    orig_db_dump = os.path.join(project_build_folder,"original_db_normalized")
    new_db_dump = os.path.join(project_build_folder,"new_db_normalized")
    html_dir = os.path.abspath(os.path.join(project_build_folder, "diffoscope"))

    with open(orig_db_dump, "w") as original_file:
        original_file.write(orig_output.decode("utf-8"))

    new_output = subprocess.check_output(arguments_new_db)

    with open(new_db_dump, "w") as new_file:
        new_file.write(new_output.decode("utf-8"))

    diffscope_dir = os.path.abspath(os.path.join(project_build_folder, "diffoscope"))

    arguments = [
        "diffoscope",
        os.path.abspath(orig_db_dump),
        os.path.abspath(new_db_dump),
        "--html-dir", diffscope_dir
    ]

    try:
        subprocess.run(arguments, check=true)
    except Exception as e:
        print("Diffscope output is at " + diffscope_dir + "/index.html")
        raise e
    finally:
        os.remove(orig_db_dump)
        os.remove(new_db_dump)

def check_dependencies():
    has_error = False

    local_dependencies = ["create_codebase_db", "dump_database"]
    system_dependencies = ["git", "diffoscope"]

    for dependency in local_dependencies:
        result = subprocess.run(["which", dependency], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        has_error |= result.returncode != 0

        if (result.returncode != 0):
            print("Check if the tool", dependency, "is installed and on your PATH")
            print("This tool is compiled by the lvtclp package from the diagram-server repository")

    for dependency in system_dependencies:
        result = subprocess.run(["which", dependency], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        has_error |= result.returncode != 0

        if (result.returncode != 0):
            print("Check if the tool", dependency, "is installed and on your PATH")
            print("Please install this dependency using your package manager.")

    if (has_error):
        sys.exit(1)

# argparse is silly and does not allow the type `bool` to be correctly
# infereed from the command line and we need to create codet o handle
# this really common case.
def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generates and compares databases for the diagram-server.")

    parser.add_argument(
        "--compare",
        help="Compares a database on the build folder against our blessed one.",
        type=str2bool,
        nargs='?',
        const=True,
        default=False
    )

    parser.add_argument(
        "--generate",
        help="generates a database, and quits",
        type=str2bool,
        nargs='?',
        const=True,
        default=True
    )

    parser.add_argument(
        "--rebuild",
        help="rebuilds the unittests source code",
        type=str2bool,
        nargs='?',
        const=True,
        default=True
    )

    parser.add_argument(
        "--bless",
        help="copies the current generated database to the system, and commits it to master.",
        type=str2bool,
        nargs='?',
        const=True,
        default=False
    )

    parser.add_argument(
        "--projects",
        help="run the tool only on the specified projects",
        type=str,
        default=""
    )

    args = parser.parse_args()

    check_dependencies()

    project_folders = []
    if len(args.projects) > 0:
        project_folders = args.projects.split(",")
    else:
        project_folders = load_folders();

    if not os.path.exists(builddir):
        os.makedirs(builddir)

    project_success = []
    project_error = []
    for project in project_folders:
        try:
            project_build_folder = create_build_folder(builddir, project)
            configure_project(project_build_folder, project)

            make_project(project_build_folder, args.rebuild)

            if (args.generate):
                print("Generating the database")
                extract_llvm_information(project_build_folder, project)

            if (args.compare):
                run_diffoscope(project_build_folder, project)
                project_success.append(project)

            if (args.bless):
                copy_current_database(project_build_folder, project)

        except Exception as e:
            project_error.append(project)

    if len(project_error) > 0:
        print("The following projects have failed:")

    for project in project_error:
        print(project)

    if len(project_success) > 0:
        print("The following projects are successfull")

    for project in project_success:
        print(project)

    if len(project_error) > 0:
        sys.exit(1)

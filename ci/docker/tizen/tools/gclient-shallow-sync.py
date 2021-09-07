#!/usr/bin/env python

import os
import sys
import subprocess
import concurrent.futures
import gclient_eval
import gclient_utils


def run_git(args, cwd):
    if not args:
        raise Exception('Please provide a git command to run')
    return subprocess.call(['git'] + args, cwd=cwd) == 0


def checkout_shallow_git(path, url, revision):
    """Checks out a shallow reference, then sets the expected branch to be
    checked out."""

    os.makedirs(path, exist_ok=True)
    run_git(['init', '--quiet'], path)
    run_git(['config', 'advice.detachedHead', 'false'], path)
    run_git(['remote', 'add', 'origin', url], path)
    run_git(['fetch', '--depth', '1', 'origin', revision], path)
    run_git(['checkout', 'FETCH_HEAD'], path)


def checkout_deps(deps):
    """Checks out the shallow refs specified in the DEPS file."""
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
        futures = []
        for deps_name, deps_data in deps.items():
            if deps_data.get('dep_type') == 'git':
                url = deps_data['url'].split('@')[0]
                revision = deps_data['url'].split('@')[1]
                futures.append(executor.submit(
                    checkout_shallow_git, deps_name, url, revision))
        for future in concurrent.futures.as_completed(futures):
            future.result()


def main(argv):
    if (len(argv) < 1):
        raise Exception('Please provide a DEPS file to update')
    deps_file = argv[0]
    if not os.path.exists(deps_file):
        raise Exception('DEPS file does not exist')
    deps_contents = gclient_utils.FileRead(deps_file)
    local_scope = gclient_eval.Parse(deps_contents, deps_file)
    checkout_deps(local_scope['deps'])


if '__main__' == __name__:
    sys.exit(main(sys.argv[1:]))

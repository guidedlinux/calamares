#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# === This file is part of Calamares - <http://github.com/calamares> ===
#
#   Copyright 2017, Hannes Schulze <projects@guidedlinux.org>
#
#   Calamares is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Calamares is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Calamares. If not, see <http://www.gnu.org/licenses/>.

import libcalamares
import subprocess
from libcalamares.utils import check_target_env_call
from libcalamares.utils import *


def run_update():
    """ Updates keyring and databases """
    check_target_env_call(['pacman-key', '--populate', 'archlinux'])


def run():
    """ Copies the gnupg dir and calls routine to update keyring and databases.

    :return:
    """
    root_mount_point = libcalamares.globalstorage.value("rootMountPoint")
    path = libcalamares.job.configuration['path']
    subprocess.check_call(["mkdir", root_mount_point + path])
    subprocess.check_call(["cp", path + "/gpg.conf", root_mount_point + path + "/"])
    subprocess.check_call(["cp", path + "/pubring.gpg", root_mount_point + path + "/"])
    subprocess.check_call(["cp", path + "/secring.gpg", root_mount_point + path + "/"])
    subprocess.check_call(["cp", path + "/trustdb.gpg", root_mount_point + path + "/"])
    subprocess.check_call(["cp", "-R", path + "/openpgp-revocs.d", root_mount_point + path + "/"])
    subprocess.check_call(["cp", "-R", path + "/private-keys-v1.d", root_mount_point + path + "/"])
    run_update()

    return None

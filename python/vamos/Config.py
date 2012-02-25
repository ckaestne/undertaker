# Copyright (C) 2012 Valentin Rothberg <valentinrothberg@googlemail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

class Config(dict):
    def __init__(self, path = None):
        dict.__init__(self)
        if path != None:
            self.readConfigFile(path)

    def readConfigFile(self, path):
        """
        Parse the file in path and add all features contained to the config.
        """
        with open(path, 'r') as stream:
            for line in stream.readlines():
                if line.startswith("CONFIG_"):
                    line = line.strip("\n")
                    split = line.split("=")
                    key = split[0].strip()
                    value = split[1].strip()
                    self.update({key:value})

    def valueOf(self, key):
        """
        Return value of key. If key is not present in the config, None is
        returned.
        """
        if key in self.keys():
            return self[key]
        else:
            return None

    def addFeature(self, key, value):
        """
        Add feature as key and value to the config.
        """
        self.update({key:value})

    def contains(self, key):
        """
        Return true if config contains key.
        """
        return key in self.keys()

    def conflict(self, other):
        """
        Return true if both Configs conflict. Configs conflict if both contain
        the same feature with different values.
        """
        for key in self.keys():
            if other.contains(key):
                if self.valueOf(key) != other.valueOf(key):
                    return True

    def showConflicts(self, other):
        """
        Returna list of conflicting features in both configs.
        """
        res = []
        for key in self.keys():
            if other.contains(key):
                if self.valueOf(key) != other.valueOf(key):
                    res.append(key)
        return res

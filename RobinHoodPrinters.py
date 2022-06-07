import regex as reg
import gdb

class RobinHoodPrinter(object):
    "Print a robin hood hashmap"

    def __init__(self, val):
        self.val = val

    def children(self):
        childCount = int(self.val['mNumElements'])
        keyvalPtr = self.val['mKeyVals']
        infoPtr = self.val['mInfo']
        result = []
        for i in range(0, childCount):
            while int(infoPtr.dereference()) == 0:
                infoPtr += 1
                keyvalPtr += 1

            if bool(self.val['is_flat']):
                keyval = ("key_" + str(i), keyvalPtr.dereference()['mData']['first'])
                result.append(keyval)
                keyval = ("value_" + str(i), keyvalPtr.dereference()['mData']['second'])
                result.append(keyval)
            elif bool(self.val['is_map']):
                keyval = ("key_" + str(i), keyvalPtr.dereference()['mData'].dereference()['first'])
                result.append(keyval)
                keyval = ("value_" + str(i), keyvalPtr.dereference()['mData'].dereference()['second'])
                result.append(keyval)
            else:
                keyval = (str(i), keyvalPtr.dereference()['mData'])
                result.append(keyval)
            
            infoPtr += 1
            keyvalPtr += 1
        return result

    def to_string(self):
        if bool(self.val['is_flat']):
            return 'robin_hood::unordered_flat_map with ' + str(int(self.val['mNumElements'])) + ' elements'
        elif bool(self.val['is_map']):
            return 'robin_hood::unordered_node_map with ' + str(int(self.val['mNumElements'])) + ' elements'
        elif bool(self.val['is_set']):
            return 'robin_hood::unordered_set with ' + str(int(self.val['mNumElements'])) + ' elements'

    def display_hint(self):
        if(bool(self.val['is_flat']) or bool(self.val['is_map'])):
            return 'map'
        else:
            return 'array'

def robin_hood_lookup_function(val):
    lookup_tag = val.type.strip_typedefs().tag
    if lookup_tag is None:
        return None
    regex = reg.compile("^robin_hood::detail::Table<.*>$")
    nodeTag = reg.compile("^robin_hood::detail::Table<.*>::DataNode<.*>$")
    iterTag = reg.compile("^robin_hood::detail::Table<.*>::Iter<.*>$")
    if regex.match(lookup_tag) and not nodeTag.match(lookup_tag) and not iterTag.match(lookup_tag):
        return RobinHoodPrinter(val)
    return None

def register_robinhood_printers():
    gdb.pretty_printers.append(robin_hood_lookup_function)
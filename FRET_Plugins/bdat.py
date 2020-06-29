
import struct
import pprint

DEBUG = False

class Struct:
	def __init__(self, src, off, args):
		self._src = src
		self._off = off
		self._args = args
	def __repr__(self):
		vl = { k:v for (k,v) in vars(self).items() if k != "_src" }
		return pprint.pformat(vl)#, indent=4, width=1)

def get_nth(val, idx):
	if isinstance(val, list):
		return val[idx] if idx >= 0 and idx < len(val) else 0
	else:
		return val if idx == 0 else 0

class InParseVariableSource:
	def __init__(self, inst):
		self.inst = inst
		self.vars = {}
	def root_query(self, type, filters):
		return []
	def subquery(self, els, field, filters):
		return []
	def get_variable(self, els, field, idx, isOffset):
		arg = self.inst._args.get(field, None)
		if arg is not None:
			return 0 if isOffset else arg
		var = self.vars.get(field, None)
		if var is not None:
			return 0 if isOffset else var
		return get_nth(getattr(self.inst, field), idx)
	def read_file(self, type, off):
		return read_builtin(type, self.inst._src, off, False, False)

class FullDataVariableSource:
	def __init__(self, inst):
		self.inst = inst
		self.vars = {}
	def root_query(self, type, filters):
		return []
	def subquery(self, els, field, filters):
		return []
	def get_variable(self, els, field, idx, isOffset):
		arg = self.inst._args.get(field, None)
		if arg is not None:
			return 0 if isOffset else arg
		var = self.vars.get(field, None)
		if var is not None:
			return 0 if isOffset else var
		# TODO
		return get_nth(getattr(els[0], field), idx)
	def read_file(self, type, off):
		return read_builtin(type, self.inst._src, off, False, False)

def read(src, off, size):
	ret = src[off:off+size]
	if DEBUG: print(off, size, ret)
	return ret

class BUILTIN_TYPE_MAP:
	char = "c"
	i8 = "b"
	u8 = "B"
	i16 = "h"
	u16 = "H"
	i32 = "i"
	u32 = "I"
	i64 = "q"
	u64 = "Q"
	f32 = "f"
	f64 = "d"
class BUILTIN_SIZE_MAP:
	char = 1
	i8 = 1
	u8 = 1
	i16 = 2
	u16 = 2
	i32 = 4
	u32 = 4
	i64 = 8
	u64 = 8
	f32 = 4
	f64 = 8

def read_builtin(type, src, off, count, readUntil0):
	if DEBUG: print("BUILTIN", type, off, count, readUntil0)
	ret = []
	n = 1 if count is False else count
	fmt = "=" + getattr(BUILTIN_TYPE_MAP, type)
	size = getattr(BUILTIN_SIZE_MAP, type)
	for i in range(n):
		val = struct.unpack(fmt, read(src, off, size))[0]
		off += size
		ret.append(val)
		if readUntil0 and (val == 0 or val == b"\0"):
			break
	if type == "char":
		ret = b"".join(ret)
	elif count is False:
		ret = ret[0]
	return ret, off

def read_struct(type, src, off, count=False, countIsMaxSize=False, args={}):
	ret = []
	n = 1 if count is False else count
	startoff = off
	for i in range(n):
		if isinstance(startoff, list):
			off = startoff[i]
			if off is None:
				ret.append(None)
				continue
		val = type(src, off, args)
		val.load()
		off = val._offend
		ret.append(val)
		if countIsMaxSize and off - startoff >= count:
			break
	if count is False:
		ret = ret[0]
	return ret, off

def me_structoff(vs, els):
	if len(els):
		return els[0]._off
	return 0

def get_preview(val):
	if isinstance(val, Struct):
		return "..."
	return str(val)

def compare_value_preview(value, preview):
	if isinstance(value, bytes):
		if value == preview:
			return True
	elif isinstance(value, list):
		value = ", ".join([get_preview(v) for v in value])
		return value.encode("utf-8") == preview
	else:
		value = get_preview(value)
		return value.encode("utf-8") == preview


def me_fpeqs(vs, els, field, text, invert):
	for el in els:
		if (compare_value_preview(getattr(el, field, None), text)) ^ invert:
			return 1
	return 0

#class Field:
#	def __init__(
#		self,
#		name,
#		type,
#		valueExpr,
#		off,
#		offExpr,
#		count,
#		countSrc,
#		countIsMaxSize,
#		individualComputedOffsets,
#		readUntil0):
#		self.name = name
#		self.type = type
#		self.valueExpr = valueExpr
#		self.off = off
#		self.offExpr = offExpr
#		self.count = count
#		self.countSrc = countSrc
#		self.countIsMaxSize = countIsMaxSize
#		self.individualComputedOffsets = individualComputedOffsets
#		self.readUntil0 = readUntil0

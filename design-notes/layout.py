
import math

class Node:
	def __init__(self, **kwargs):
		self.x0 = 0
		self.x1 = 10
		self.children = []
		self.text = None
		self.w = None
		self.h = None
		self.__dict__.update(kwargs)
	def render(self, lines, lev):
		lines[lev][self.x0] = "<"
		lines[lev][self.x1 - 1] = ">"
		if self.text:
			for i, c in enumerate(self.text):
				lines[lev][self.x0 + i + 1] = c
		for ch in self.children:
			ch.render(lines, lev + 1)
	def getminmaxw(self, notself=False):
		if not notself:
			if self.w is not None:
				return self.w, self.w
			if self.text:
				return len(self.text) + 2, len(self.text) + 2
		minw = 0
		for ch in self.children:
			minw += ch.getminmaxw()[0]
		return minw, 5000
#

def render(node):
	lines = [[c for c in " " * 50] for i in range(5)]
	node.render(lines, 0)
	for line in lines:
		print("".join(line))
#

def center_layout(node):
	w = node.x1 - node.x0
	minw = node.getminmaxw(True)[0]
	leftover = w - minw
	p = node.x0 + leftover // 2
	for ch in node.children:
		ch.x0 = math.floor(p)
		p += ch.getminmaxw()[0]
		ch.x1 = math.floor(p)
#

def horizontal_layout(node, w):
	info = []
	minw = 0
	maxw = 5000
	for ch in node.children:
		(chminw, chmaxw) = ch.getminmaxw()
		minw += chminw
		info.append([ch, chminw, chmaxw, chminw])
	if w < minw: w = minw
	if w > maxw: w = maxw
	node.x0 = 0
	node.x1 = w
	leftover = w - minw
	# distribute leftover according to allowed range (starting with smallest ranges)
	sinfo = sorted(info, key=lambda v:v[2] - v[1])
	nitems = len(info)
	for chi in sinfo:
		curlo = leftover / nitems
		if curlo > chi[2] - chi[1]: curlo = chi[2] - chi[1]
		chi[3] += curlo
		leftover -= curlo
		nitems -= 1
	p = 0
	for chi in info:
		ch = chi[0]
		ch.x0 = math.floor(p)
		p += chi[3]
		ch.x1 = math.floor(p)
		center_layout(ch)
#

test1 = Node(children=[
	Node(children=[Node(text="txt")], w=7),
	Node(children=[Node(text="longer text"), Node(text="2")]),
])
horizontal_layout(test1, 45)
render(test1)

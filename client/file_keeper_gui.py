import wx, threading, socket, Queue, select, re, os, time, subprocess, sys

(
FILE_MESSAGE_INVALID_TYPE,
FILE_MESSAGE_NEW,
FILE_MESSAGE_DELETED,
FILE_MESSAGE_VERSION,
FILE_MESSAGE_ROOT_PATH,
FILE_MESSAGE_REVERT,
FILE_MESSAGE_REVERT_ABORT,
FILE_MESSAGE_REVERT_CONFIRM,
FILE_MESSSAGE_VERSIONS
) = range(0, 9)

class FileMessage:
	def __init__(self, str=None, command=None, file=None,timestamp=None):
		if (str is not None):
			p = re.compile('<command>(\\d+)</command><file>(.+)</file>(<timestamp>(\\d+)</timestamp>){0,1}',
			re.IGNORECASE)
			m = p.match(str)
			self.command = int(m.group(1))
			self.file = m.group(2)
			aux = m.group(4)
			if (aux is not None):
				self.timestamp = long(aux)
			else:
				self.timestamp = 0
		else:
			self.command = command
			self.file = file
			if (timestamp is not None):
				self.timestamp = self.StringTimeToUnixTime(timestamp)
			else:
				self.timestamp = 0

	def StringTimeToUnixTime(self, str):
		return long(time.mktime(time.strptime(str, "%d/%m/%Y %H:%M:%S")))

	def __str__(self):
		buf = "<command>"+ str(self.command) + "</command>" +"<file>" + self.file + "</file>"
		if (self.timestamp is not None):
			buf += "<timestamp>" + str(self.timestamp) + "</timestamp>"
		buf += "\n"
		return buf

	def TimeStampToHumanForm(self):
		return time.strftime("%d/%m/%Y %H:%M:%S", time.localtime(self.timestamp))

class FileKeeperMainWindow(wx.Frame):
	def __init__(self, title, size):
		wx.Frame.__init__(self, None, title = title, size = size)

		self.socketWatcher = SocketWatcher(self)

		self.tree = wx.TreeCtrl(self, -1,
			style=wx.TR_HAS_BUTTONS|wx.TR_DEFAULT_STYLE|wx.SUNKEN_BORDER)

		self.versionList = wx.ListBox(self, -1, style=wx.LB_SINGLE)

		mainSizer = wx.BoxSizer(wx.VERTICAL)

		listSizer = wx.BoxSizer(wx.HORIZONTAL)
		listSizer.Add(self.tree, 1, wx.EXPAND, 0)
		listSizer.Add(self.versionList, 2, wx.EXPAND, 0)

		buttonSizer = wx.BoxSizer(wx.HORIZONTAL)
		btn1 = wx.Button(self, -1, "Preview the selected date")
		btn2 = wx.Button(self, -1, "Revert to select date")
		btn3 = wx.Button(self, -1, "Cancel")
		buttonSizer.Add(btn1, 1, wx.EXPAND, 0)
		buttonSizer.Add(btn2, 1, wx.EXPAND, 0)
		buttonSizer.Add(btn3, 1, wx.EXPAND, 0)

		mainSizer.Add(listSizer, 1, wx.EXPAND, 0)
		mainSizer.Add(buttonSizer, 1, wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(mainSizer)

		self.Bind(wx.EVT_CLOSE, self.OnClose)
		self.tree.Bind(wx.EVT_TREE_SEL_CHANGED, self.OnFileSelected)

		btn1.Bind(wx.EVT_BUTTON, self.OnPreviewClicked)
		btn2.Bind(wx.EVT_BUTTON, self.OnRevertClicked)
		btn3.Bind(wx.EVT_BUTTON, self.OnCancelClicked)

		self.mustAbortRevert = False
		self.currentFile = ""

		self.Layout()
		self.Centre()
		self.Show()
		self.socketWatcher.Connect(8001)
		self.socketWatcher.start()

	def OnCancelClicked(self, evt):
		if self.mustAbortRevert == False:
			return

		self.socketWatcher.SendCommand(
		FileMessage(command=FILE_MESSAGE_REVERT_ABORT,file=self.currentFile))
		self.mustAbortRevert = False

	def OnFileSelected(self, evt):
		self.versionList.Clear()
		file = self.tree.GetItemText(evt.GetItem())
		self.socketWatcher.SendCommand(
		FileMessage(command=FILE_MESSSAGE_VERSIONS,file=file))

		if self.mustAbortRevert:
			self.mustAbortRevert = False
			self.socketWatcher.SendCommand(
			FileMessage(command=FILE_MESSAGE_REVERT_ABORT,file=self.currentFile))

		self.currentFile = file

	def OpenProgram(self, file):
		if sys.platform.startswith('darwin'):
			subprocess.call(('open', file))
		elif os.name == 'nt':
			os.startfile(file)
		elif os.name == 'posix':
			subprocess.call(('xdg-open', file))

	def RevertToSelectedVersion(self, commit):
		selection = self.versionList.GetSelections()
		if (len(selection) == 0):
			return #todo create a popup

		if commit == True:
			self.mustAbortRevert = False
			command = FILE_MESSAGE_REVERT_CONFIRM
		else:
			self.mustAbortRevert = True
			command = FILE_MESSAGE_REVERT

		itemStr = self.versionList.GetString(selection[0])
		self.socketWatcher.SendCommand(
		FileMessage(command=command,file=self.currentFile, timestamp=itemStr))
		if (command == FILE_MESSAGE_REVERT):
			self.OpenProgram(self.currentFile)
		else:
			self.versionList.Clear()
			self.socketWatcher.SendCommand(
			FileMessage(command=FILE_MESSSAGE_VERSIONS,file=self.currentFile))

	def OnPreviewClicked(self, evt):
		self.RevertToSelectedVersion(False)

	def OnRevertClicked(self, evt):
		self.RevertToSelectedVersion(True)

	def OnClose(self, evt):
		self.Destroy()
		self.socketWatcher.Terminate()

	def PopulateView(self, root, path):
		for f in os.listdir(path):
			if f == ".db":
				continue
			aux = os.path.join(path, f)
			id = self.tree.AppendItem(root, aux)
			if os.path.isdir(aux):
				self.PopulateView(id, aux)

	def OnNewMessage(self, msg):
		if msg.command == FILE_MESSAGE_ROOT_PATH:
			root = self.tree.AddRoot(msg.file)
			self.PopulateView(root, msg.file)
			self.tree.Expand(root)
		elif msg.command == FILE_MESSAGE_VERSION:
			current = self.tree.GetItemText(self.tree.GetSelection())
			if (current == msg.file):
				self.versionList.Append(msg.TimeStampToHumanForm())

class SocketWatcher(threading.Thread):
	def __init__(self, gui):
		threading.Thread.__init__(self)
		self.daemon = True
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.eventStop = threading.Event()
		self.send_queue = Queue.Queue()
		self.gui = gui

	def Terminate(self):
		self.eventStop.set()
		self.sock.shutdown(socket.SHUT_RDWR)
		self.sock.close()

	def Connect(self, port):
		self.sock.connect(("localhost", port))
		self.sock.setblocking(0)

	def SendCommand(self, msg):
		self.send_queue.put(msg, False)

	def run(self):
		while not self.eventStop.is_set():
			try:
				msg = self.send_queue.get(True, 0.5) #check if there is something to send
				self.sock.send(msg.__str__())
			except Queue.Empty as e:
				pass
			can_read = select.select([self.sock], [], [], 0.5) #Do not block, check if we have data
			if can_read[0]:
				buf = self.sock.recv(4096)
				while buf.find('\n') != -1:
					line, buf = buf.split('\n', 1)
					wx.CallAfter(self.gui.OnNewMessage, FileMessage(str=line))


if __name__ == "__main__":
	app = wx.App()
	FileKeeperMainWindow("File Keeper", (1024, 768))
	app.MainLoop()

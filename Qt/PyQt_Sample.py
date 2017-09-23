from PyQt5.Qt import *
from PyQt5.QtWidgets import *

class Sample(QWidget):

    def __init__(self, parent = None):
        QWidget.__init__(self, parent)
        self.button = QPushButton("button", self)
        self.label = QLabel("sample", self)
        self.label.setAlignment(Qt.AlignCenter)
        self.label.setVisible(False)
        self.layout = QHBoxLayout(self)
        self.layout.addWidget(self.button)
        self.layout.addWidget(self.label)
        self.resize(500, 100)
        self.button.clicked.connect(self.toggleMsg)

    def toggleMsg(self):
        self.label.setVisible(not self.label.isVisible())

import sys
app = QApplication(sys.argv)
smp = Sample(None)
smp.show()
sys.exit(app.exec())


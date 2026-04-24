import sys

from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import QApplication

from main_window import MainWindow


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    app.setFont(QFont("Arial", 11))
    app.setStyleSheet(
        """
        QWidget {
            background-color: #101010;
            color: #FFFFFF;
        }
        QLabel {
            color: #FFFFFF;
        }
        """
    )

    window = MainWindow()
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
using System.Windows;

namespace FirstEngineEditor.Views
{
    public partial class InputDialog : Window
    {
        public string Answer { get; set; } = string.Empty;
        public string Prompt { get; set; } = string.Empty;

        public InputDialog(string prompt, string title, string defaultValue = "")
        {
            InitializeComponent();
            Title = title;
            Prompt = prompt;
            Answer = defaultValue;
            DataContext = this;
            InputTextBox.Focus();
            InputTextBox.SelectAll();
        }

        private void OnOkClick(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void OnCancelClick(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

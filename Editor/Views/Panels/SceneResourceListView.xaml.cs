using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using FirstEngineEditor.ViewModels.Panels;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;

namespace FirstEngineEditor.Views.Panels
{
    public partial class SceneResourceListView : System.Windows.Controls.UserControl
    {
        private SceneResourceListViewModel? _viewModel;
        private ISceneManagementService? _sceneManagementService;
        private SceneResourceItem? _contextMenuItem;
        private SceneResourceItem? _draggedItem;
        private System.Windows.Point _dragStartPoint;

        public SceneResourceListView()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            _viewModel = DataContext as SceneResourceListViewModel;
            _sceneManagementService = ServiceLocator.Get<ISceneManagementService>();
        }

        private void OnSaveScene(object sender, RoutedEventArgs e)
        {
            if (_sceneManagementService == null || _viewModel == null) 
            {
                System.Windows.MessageBox.Show("Scene management service or view model is not available", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            try
            {
                // First, save the scene data from C# side (this ensures all editor changes are saved)
                // This saves the JSON file directly with all the latest changes
                _viewModel.SaveSceneData();

                // Note: We don't call SaveSceneAsync here because it would save from C++ engine's memory,
                // which might not have the latest changes. The C# side JSON save is the source of truth.
                // If the C++ engine needs to be updated, the scene should be reloaded.
                
                System.Windows.MessageBox.Show("Scene saved successfully", "Success", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnSaveScene: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error saving scene: {ex.Message}\n\nStack trace:\n{ex.StackTrace}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnRefresh(object sender, RoutedEventArgs e)
        {
            try
            {
                _viewModel?.RefreshSceneResources();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnRefresh: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error refreshing scene: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnSelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            if (e.NewValue is SceneResourceItem item)
            {
                _viewModel?.SelectItem(item);
            }
        }

        // ========== Context Menu ==========

        private void OnTreeViewPreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            var treeViewItem = FindAncestor<System.Windows.Controls.TreeViewItem>((DependencyObject)e.OriginalSource);
            if (treeViewItem != null)
            {
                _contextMenuItem = treeViewItem.DataContext as SceneResourceItem;
                UpdateContextMenu();
            }
        }

        private void UpdateContextMenu()
        {
            if (_contextMenuItem == null || _viewModel == null)
                return;

            // Update menu items visibility and enabled state
            AddLevelMenuItem.Visibility = _viewModel.CanAddSceneLevel(_contextMenuItem) 
                ? Visibility.Visible : Visibility.Collapsed;
            AddEntityMenuItem.Visibility = _viewModel.CanAddEntity(_contextMenuItem) 
                ? Visibility.Visible : Visibility.Collapsed;
            RenameMenuItem.Visibility = _viewModel.CanRename(_contextMenuItem) 
                ? Visibility.Visible : Visibility.Collapsed;
            DeleteMenuItem.Visibility = (_viewModel.CanDeleteSceneLevel(_contextMenuItem) || 
                                        _viewModel.CanDeleteEntity(_contextMenuItem) ||
                                        _viewModel.CanDeleteResource(_contextMenuItem)) 
                ? Visibility.Visible : Visibility.Collapsed;

            AddLevelMenuItem.IsEnabled = _viewModel.CanAddSceneLevel(_contextMenuItem);
            AddEntityMenuItem.IsEnabled = _viewModel.CanAddEntity(_contextMenuItem);
            RenameMenuItem.IsEnabled = _viewModel.CanRename(_contextMenuItem);
            DeleteMenuItem.IsEnabled = _viewModel.CanDeleteSceneLevel(_contextMenuItem) || 
                                     _viewModel.CanDeleteEntity(_contextMenuItem) ||
                                     _viewModel.CanDeleteResource(_contextMenuItem);
        }

        private void OnAddLevel(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_contextMenuItem == null || _viewModel == null)
                    return;

                var dialog = new Views.InputDialog("Enter level name:", "Add Level", "NewLevel");
                if (dialog.ShowDialog() == true && !string.IsNullOrEmpty(dialog.Answer))
                {
                    _viewModel.AddSceneLevel(_contextMenuItem, dialog.Answer);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnAddLevel: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error adding level: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnAddEntity(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_contextMenuItem == null || _viewModel == null)
                    return;

                var dialog = new Views.InputDialog("Enter entity name:", "Add Entity", "NewEntity");
                if (dialog.ShowDialog() == true && !string.IsNullOrEmpty(dialog.Answer))
                {
                    _viewModel.AddEntity(_contextMenuItem, dialog.Answer);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnAddEntity: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error adding entity: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnRename(object sender, RoutedEventArgs e)
        {
            if (_contextMenuItem == null || _viewModel == null)
                return;

            var dialog = new Views.InputDialog("Enter new name:", "Rename", _contextMenuItem.Name);
            if (dialog.ShowDialog() == true && !string.IsNullOrEmpty(dialog.Answer))
            {
                _viewModel.RenameItem(_contextMenuItem, dialog.Answer);
            }
        }

        private void OnDelete(object sender, RoutedEventArgs e)
        {
            if (_contextMenuItem == null || _viewModel == null)
                return;

            var result = System.Windows.MessageBox.Show(
                $"Are you sure you want to delete '{_contextMenuItem.Name}'?",
                "Confirm Delete",
                System.Windows.MessageBoxButton.YesNo,
                System.Windows.MessageBoxImage.Question);

            if (result == System.Windows.MessageBoxResult.Yes)
            {
                if (_viewModel.CanDeleteSceneLevel(_contextMenuItem))
                {
                    _viewModel.DeleteSceneLevel(_contextMenuItem);
                }
                else if (_viewModel.CanDeleteEntity(_contextMenuItem))
                {
                    _viewModel.DeleteEntity(_contextMenuItem);
                }
                else if (_viewModel.CanDeleteResource(_contextMenuItem))
                {
                    _viewModel.DeleteResource(_contextMenuItem);
                }
            }
        }

        // ========== Drag and Drop ==========

        private void OnItemPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            _dragStartPoint = e.GetPosition(null);
        }

        private void OnItemMouseMove(object sender, System.Windows.Input.MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                var item = FindAncestor<System.Windows.Controls.TreeViewItem>((DependencyObject)sender);
                if (item != null && item.DataContext is SceneResourceItem sceneItem)
                {
                    System.Windows.Point currentPoint = e.GetPosition(null);
                    System.Windows.Vector diff = _dragStartPoint - currentPoint;

                    if (Math.Abs(diff.X) > SystemParameters.MinimumHorizontalDragDistance ||
                        Math.Abs(diff.Y) > SystemParameters.MinimumVerticalDragDistance)
                    {
                        _draggedItem = sceneItem;
                        var dataObject = new System.Windows.DataObject("SceneResourceItem", sceneItem);
                        System.Windows.DragDrop.DoDragDrop(item, dataObject, System.Windows.DragDropEffects.Move);
                        _draggedItem = null;
                    }
                }
            }
        }

        private void OnDragOver(object sender, System.Windows.DragEventArgs e)
        {
            if (_viewModel == null)
                return;

            // Check if dragging from resource browser
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                if (e.Data.GetData("ResourceBrowserItem") is ResourceBrowserItem resourceItem)
                {
                    if (resourceItem.Type == "Model")
                    {
                        // Can drop model on Entity
                        var targetItem = GetItemUnderMouse(e);
                        if (targetItem != null && targetItem.Type == SceneResourceType.Entity)
                        {
                            e.Effects = System.Windows.DragDropEffects.Link;
                            e.Handled = true;
                            return;
                        }
                    }
                }
            }

            // Check if dragging scene resource item
            if (e.Data.GetDataPresent("SceneResourceItem") && _draggedItem != null)
            {
                var targetItem = GetItemUnderMouse(e);
                if (targetItem != null && targetItem != _draggedItem)
                {
                    // Check if can move
                    if (_draggedItem.Type == SceneResourceType.SceneLevel && 
                        _viewModel.CanMoveSceneLevel(_draggedItem, targetItem))
                    {
                        e.Effects = System.Windows.DragDropEffects.Move;
                        e.Handled = true;
                        return;
                    }
                    else if (_draggedItem.Type == SceneResourceType.Entity && 
                             _viewModel.CanMoveEntity(_draggedItem, targetItem))
                    {
                        e.Effects = System.Windows.DragDropEffects.Move;
                        e.Handled = true;
                        return;
                    }
                }
            }

            e.Effects = System.Windows.DragDropEffects.None;
        }

        private void OnDrop(object sender, System.Windows.DragEventArgs e)
        {
            if (_viewModel == null)
                return;

            var targetItem = GetItemUnderMouse(e);
            if (targetItem == null)
            {
                // If no target item, try to get selected item as fallback
                targetItem = _viewModel.SelectedItem;
            }

            if (targetItem == null)
                return;

            // Handle resource browser drop (Model)
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                if (e.Data.GetData("ResourceBrowserItem") is ResourceBrowserItem resourceItem)
                {
                    if (resourceItem.Type == "Model")
                    {
                        // Can drop on Entity or if Entity is selected
                        SceneResourceItem? entityItem = null;
                        if (targetItem.Type == SceneResourceType.Entity)
                        {
                            entityItem = targetItem;
                        }
                        else if (targetItem.Type == SceneResourceType.Resource && targetItem.Parent?.Type == SceneResourceType.Entity)
                        {
                            entityItem = targetItem.Parent;
                        }
                        else if (_viewModel.SelectedItem?.Type == SceneResourceType.Entity)
                        {
                            entityItem = _viewModel.SelectedItem;
                        }

                        if (entityItem != null)
                        {
                            // Extract model ID from resource item
                            int modelId = resourceItem.Id;
                            if (modelId > 0)
                            {
                                _viewModel.AddResourceToEntity(entityItem, modelId);
                                e.Handled = true;
                                return;
                            }
                        }
                    }
                }
            }

            // Handle scene resource item drag
            if (e.Data.GetDataPresent("SceneResourceItem") && _draggedItem != null)
            {
                if (_draggedItem.Type == SceneResourceType.SceneLevel && 
                    _viewModel.CanMoveSceneLevel(_draggedItem, targetItem))
                {
                    _viewModel.MoveSceneLevel(_draggedItem, targetItem);
                    e.Handled = true;
                }
                else if (_draggedItem.Type == SceneResourceType.Entity && 
                         _viewModel.CanMoveEntity(_draggedItem, targetItem))
                {
                    _viewModel.MoveEntity(_draggedItem, targetItem);
                    e.Handled = true;
                }
            }
        }

        private SceneResourceItem? GetItemUnderMouse(System.Windows.DragEventArgs e)
        {
            // Try to get the item from the drop position
            var treeView = SceneResourceTreeView;
            if (treeView == null)
                return null;

            var position = e.GetPosition(treeView);
            var hitTest = System.Windows.Media.VisualTreeHelper.HitTest(treeView, position);
            if (hitTest != null)
            {
                var treeViewItem = FindAncestor<System.Windows.Controls.TreeViewItem>(hitTest.VisualHit);
                if (treeViewItem != null)
                {
                    return treeViewItem.DataContext as SceneResourceItem;
                }
            }

            // Fallback: try to get from original source
            var originalSource = e.OriginalSource as DependencyObject;
            if (originalSource != null)
            {
                var treeViewItem = FindAncestor<System.Windows.Controls.TreeViewItem>(originalSource);
                if (treeViewItem != null)
                {
                    return treeViewItem.DataContext as SceneResourceItem;
                }
            }

            // Last resort: return selected item
            return _viewModel?.SelectedItem;
        }

        private static T? FindAncestor<T>(DependencyObject current) where T : DependencyObject
        {
            while (current != null)
            {
                if (current is T ancestor)
                    return ancestor;
                current = System.Windows.Media.VisualTreeHelper.GetParent(current);
            }
            return null;
        }
    }
}

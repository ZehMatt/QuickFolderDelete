# QuickFolderDelete
Are you tired of watching explorer delete file by file when deleting big directories? This tool solves this problem by using rmdir instead of deleting files individually.
I wanted to have a nice shell integration so when using this it will show a question prompt first. When it fails to delete the directory it shows the error from rmdir and
also allows to retry the attempt.

# Shell Integration.
1. Open the registry and create a new Key under `HKEY_CLASSES_ROOT\Directory\shell` call it `QuickFolderDelete`. 
2. Modify the `Default` of `Computer\HKEY_CLASSES_ROOT\Directory\shell\QuickFolderDelete` and set the value to `Quick Delete`
3. Create a new key called `command` under `Computer\HKEY_CLASSES_ROOT\Directory\shell\QuickFolderDelete`
4. Set the `Default` to `"<PATH_TO_QUICKFOLDERDELETE>" %1`, the it should look like following: `"F:\Tools\QuickFolderDelete.exe" "%1"`
That's all. Now you should see a new menu when right clicking on folders called `Quick Delete`

# Compiling
The project uses CMake, just invoke `cmake . -B build` in the root directory and it will create the Visual Studio project.

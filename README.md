# Refresh All Blueprint Nodes
This Unreal Engine editor plugin allows you to bulk-refresh all Blueprint assets in your project without needing to open each one individually.
It is designed to automatically fix "dirty" Blueprint errors and compilation issues that often appear after changing C++ based `Structs` or function signatures.

*Disclaimer: This is an experimental plugin developed with the assistance of Claude AI. Please back up your project before use, as automated bulk operations can have unintended consequences.*

## Features
Based on the code analysis, the plugin offers two primary functions:

### 1. Refresh Entire Project (Global)
This function finds and refreshes **all** Blueprint assets located under your project's `/Game/` (Content) folder.
* **How to run:**
    * Main Menu: **Tools** -> **Refresh All Blueprint Nodes**
    * Main Toolbar: Plugin icon

### 2. Refresh by Folder (Specific)
This function refreshes only the Blueprints within the specific folder(s) you select, including all subfolders.
* **How to run:**
    * In the Content Browser, right-click on a folder.
    * Select **"Refresh Blueprint Nodes in Folder"** from the context menu.

## How It Works
When either action is initiated:
1.  **Asks for Confirmation:** A dialog box appears showing how many Blueprints will be affected.
2.  **Safe:** It operates only on `/Game/` (project content) and will not modify `/Engine/` (engine content) files.
3.  **Shows Progress:** A cancelable progress bar (`Slow Task`) is displayed during the operation.
4.  **Reports Results:** A summary is provided upon completion, detailing how many Blueprints were processed successfully and how many encountered errors.

## Installation
1.  Clone or download this repository into your project's `Plugins` folder.
2.  Restart your project.
3.  Ensure the plugin (RefreshAllNodes) is enabled in the **Edit -> Plugins** menu.

## Known Limitations
⚠️ UI Thread Block - The editor may freeze in large projects (UE limitation) 
⚠️ Manual Save Required - The user must save manually (by design) 
⚠️ No Undo - Blueprint changes cannot be undone (UE limitation)

## Usage Recommendations
1. Use source control
2. Commit before running
3. Test with small folders

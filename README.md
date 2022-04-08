# Desktop+ Browser
Browser component for the Desktop+ VR overlay application, using the Chromium Embedded Framework.

## Usage

### Steam

Switch to the "Desktop+ NewUI Preview + Browser" beta branch.  
Browser overlays will now be available within Desktop+.

### Release Archive

Download and extract the latest archive from the [releases page](https://github.com/elvissteinjr/DesktopPlusBrowser/releases). Extract the `DesktopPlusBrowser` folder (as-is, not just the contents) into the Desktop+ install directory.  
Make sure to pair the download with a version of Desktop+ that supports the respective Desktop+ Browser release. See the release entry for this information.

If everything is working, browser overlays will now be available within Desktop+. DesktopPlusBrowser.exe will be run by Desktop+ on demand (don't launch it yourself).

### Building from Source

Desktop+ Browser is built as part of Desktop+.  
Add the `assets_browser` and `src` folders to a copy of the Desktop+ repository before trying to build the application.  
Alternatively you can also take the `Shared` folder from the Desktop+ repository and add it to the `src` folder to keep it more separate. This will require moving the build's output to the Desktop+ folder afterwards, however.

With the required files in place, the Visual Studio 2019 Solution builds out of the box with no further external dependencies.

A build of CEF is included in the `assets_browser` directory as convenience. If you want to build CEF yourself, keep in mind Desktop+ Browser requires the [CEF fork maintained by the OBS Project](https://github.com/obsproject/cef) (a big thanks for their phenomenal work here!).

Other compilers likely work as well, but are neither tested nor have a build configuration.

## License

This software is licensed under the GPL 3.0.  
Desktop+ Browser includes work of third-party projects. For their licenses, see [here](assets_browser/third-party_licenses.txt).

# Development guidelines

## Project structure

Project is organized in folders:
- root folder contains readme, licence, project default configuration and flash partitioning
- main/ contains main components and project configuration
- doc/ contains documentation (in addition to readme)
- config/ contains classes related to configuration management
- helpers/ contains classes and functions related to specific features like command line management, configuration and devices storage, MQTT frontend, network management.
- diagral/ contains main implementation of the Diagral protocol over UART

> [!NOTE]
> If you want to reuse Diagral protocol management with a different frontend, you can directly use diagral/ in your project. Please add a link to my project.

## Project documentation

Most of the classes, methods and members are directly described in source code using doxygen.

## External components

Most of the source code not directly related to Diagral has been written based on examples from ESP-IDF SDK: NVS and configuration, Ethernet and Wifi, MQTT, command line tools...  
You can also read ESP-IDF SDK documentation and examples documentation to learn more about these features.

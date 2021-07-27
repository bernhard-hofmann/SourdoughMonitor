# Sourdough Monitor

## Overview

This sourdough monitor is an ESP8266-based project with components to monitor humidty, temperature, and the height of the starter in the container that the monitor is installed atop. Basic information is shown on an OLED 128x32 screen and data is sent to an IoT cloud store.

## Design Choices

Whilst this project was inspired by [Justin Lam's Levain Monitor](https://www.justinmklam.com/posts/2021/02/levain-monitor/), it is mostly a completely new codebase and design because we could not get the PlatformIO code to work. You can also find [his project on GitHub](https://github.com/justinmklam/iot-sourdough-starter-monitor).

1. The system must be funcitonal/useful even if no cloud storage or web app is used. This makes it more accessible to beginners or those who want a simpler option.
1. No decision is made with respect to power source. The sensors are all powered from the ESP, but the ESP could be powered by battery, USB, or power adapter.
1. We will to use as many free services as possible to keep costs down.
1. We hope to create a PCB but the initial version, at least, may be a proto-board or breadboard based solution.

## Services used

1. [IoTPlotter](http://iotplotter.com/) "is a service which collects data from your IoT devices for long-term graph plotting and storage".
1. [Firebase](https://firebase.google.com/) provides a free (up to 1GiB) data store for long term storage. The intention is to move the data out of IoTPlotter (which is in Beta at the time of writing) and into a production-ready database of our own for long term storage.

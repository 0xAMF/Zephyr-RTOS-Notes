## Building and Running
This example was made for the stm32f407 discovery board.
If you are building for a different board, i suggest to look at the ADC samples in the zephyr repo.

**Build**
```bash
west build /path/to/adc_demo -b stm32f4_disco --pristine
```
**Flash**
```bash
west flash /path/to/adc_demo/build/
```
- Note that if you have the environement variables set up and you are inside the application directory, you don't have to pass the path to the application directory in the build.
- Since i'm the application is a freestanding application outside the zephyr repo, i'm passing in the application's path because i'm building while i'm inside the zephyr directory.

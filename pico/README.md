You might have trouble with the pico extras. Worst comes to worst:
```
cd "$PICO_SDK_PATH/.."
git clone https://github.com/raspberrypi/pico-extras.git
cd -
```

and then clean/reconfigure cmake

make sure you also build the pico sdk, extras, and fft libraries.

If you get the error: `undefined reference to 'vtable for strum_irq::StrumIrq'`
clean cmake and recompile.
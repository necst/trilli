## AE: Matlab Image Registration Toolkit

We provide a code-base to evaluate matlab image registration toolkit. 

As matlab itself offer 1+1 optimizer only, we use such code-base and matlab profiler to evaluate the transformation and rigid step time. 

*To reproduce the complete 3D image registration, please contact the authors or refer to [ATHENA](https://github.com/necst/athena). We will privately send you the dataset, that are required to ensure a correct image registration step.*

### How To evaluate
All these steps are meant to be executed into MATLAB environment, through command line. 

# Step 1: Run Profiler
```
profile on
```
# Step 2: Run image registration

Run the full image registration

# Step 3: Stop profiler
```
profile off
```

# Step 4: Stop profiler
```
profile off
```

# Step 5: Visualize the Report
```
profile viewer
```



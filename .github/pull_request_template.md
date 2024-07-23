
Our CI tests check formatting automating. If such a test fails, please consider running the bash script:

```bash
bash script/run-clangcldocker.sh
```

Make sure that you have [docker installed and running](https://docs.docker.com/engine/install/) on your system. Most Linux distributions support docker though some (like RedHat) have the equivalent (Podman). Users of Apple systems may want to [consider OrbStack](https://orbstack.dev). You do not need to familiar with docker, you just need to make sure that you are have it running.

If you are unable to format the code, we may format it for you.

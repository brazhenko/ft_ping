# ft_ping

### Prepare environment

```bash
docker build -t clion/remote -f Dockerfile.remote .
docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name remote_env clion/remote
```


### Usage

```bash

```

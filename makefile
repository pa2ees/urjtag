GITBASE:=$(shell git rev-parse --show-toplevel)
DOCKER=docker run -v $(GITBASE):$(GITBASE) --rm -w=$(PWD) travisci/ubuntu-ruby:16.04

aarch64:
	$(DOCKER) ./ci/install-$@.sh
	$(DOCKER) ./ci/build-$@.sh

armv7l:
	$(DOCKER) ./ci/install-$@.sh
	$(DOCKER) ./ci/build-$@.sh

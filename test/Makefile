all:
	make -C helloworld
	make -C asynctst
	make -C smoketest
	make -C actcancel
	make -C evtest
	make -C inproc
	make -C katest
	make -C prtest

release:
	make -C helloworld release
	make -C asynctst release
	make -C smoketest release
	make -C actcancel release
	make -C evtest release
	make -C inproc release
	make -C katest release
	make -C prtest release

clean:
	make -C helloworld clean
	make -C asynctst clean
	make -C smoketest clean
	make -C actcancel clean
	make -C evtest clean
	make -C inproc clean
	make -C katest clean
	make -C prtest clean

.PHONY: release clean

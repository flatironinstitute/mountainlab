TEMPLATE = subdirs

# usage:
# qmake
# qmake "COMPONENTS = mproc"

isEmpty(COMPONENTS) {
    COMPONENTS = mda mdaconvert mproc prv
}

isEmpty(GUI) {
    GUI = on
}

CONFIG += ordered

defineReplace(ifcomponent) {
  contains(COMPONENTS, $$1) {
    message(Enabling $$1)
    return($$2)
  }
  return("")
}

SUBDIRS += cpp/mlcommon/src/mlcommon.pro
SUBDIRS += cpp/mlconfig/src/mlconfig.pro
SUBDIRS += $$ifcomponent(mdaconvert,cpp/mdaconvert/src/mdaconvert.pro)
SUBDIRS += $$ifcomponent(mda,cpp/mda/src/mda.pro)
SUBDIRS += $$ifcomponent(mproc,cpp/mproc/src/mproc.pro)
SUBDIRS += $$ifcomponent(prv,cpp/prv/src/prv.pro)

DISTFILES += features/*
DISTFILES += debian/*

deb.target = deb
deb.commands = debuild $(DEBUILD_OPTS) -us -uc

QMAKE_EXTRA_TARGETS += deb

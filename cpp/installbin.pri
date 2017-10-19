prefix=$${PREFIX}
isEmpty(prefix) {
    prefix="$${ML_DIR}"
}
target.path = "$$prefix/bin"
INSTALLS += target

for(ff,EXTRA_INSTALLS) {
    install0.files = "$$ff"
    install0.path = "$$prefix/bin"
    INSTALLS += install0
}

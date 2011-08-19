polkit_policydir = $(datadir)/polkit-1/actions
polkit_policy_DATA =					\
	org.mate.power.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.mate.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

.in2.in:
	sed "s|[@]sbindir@|${sbindir}|" $< > $@

EXTRA_DIST = org.mate.power.policy.in2
DISTCLEANFILES = org.mate.power.policy org.mate.power.policy.in

clean-local :
	rm -f *~


###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make SUBTARGET=tiny to build
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame

AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

OBJDIRS += \
	$(AUDIO) \
	$(DRIVERS) \
	$(LAYOUT) \
	$(MACHINE) \
	$(VIDEO) \



#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

CPUS += Z80
CPUS += M6502
CPUS += MCS48
CPUS += MCS51
CPUS += M6800
CPUS += M6809
CPUS += M680X0
CPUS += TMS9900
CPUS += COP400
CPUS += MIPS
CPUS += PSX
CPUS += TMS340X0
CPUS += TMS9900
CPUS += Z8000
CPUS += Z8001
CPUS += TMS32010
CPUS += TMS32025
CPUS += TMS32031
CPUS += TMS32051
CPUS += TMS57002
CPUS += ADSP21XX
CPUS += I86
CPUS += I386
CPUS += PIC16C5X
CPUS += PIC16C62X
CPUS += SH2
CPUS += SH4
CPUS += MN10200

# compile fixe 
SOUNDS += BSMT2000
SOUNDS += SPU
SOUNDS += CDDA
SOUNDS += C352
SOUNDS += TMS36XX
SOUNDS += TMS3615
SOUNDS += TMS5110
SOUNDS += TMS5220
SOUNDS += QSOUND
SOUNDS += YM2203
SOUNDS += YM2413
SOUNDS += YM2608
SOUNDS += YM2610
SOUNDS += YM2610B
SOUNDS += YMZ280B
SOUNDS += YMF271
SOUNDS += MSM5205
SOUNDS += SN76496
SOUNDS += ZSG2

#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

SOUNDS += SAMPLES
SOUNDS += DAC
SOUNDS += DISCRETE
SOUNDS += AY8910
SOUNDS += YM2151
SOUNDS += ASTROCADE
SOUNDS += TMS5220
SOUNDS += OKIM6295
SOUNDS += HC55516
SOUNDS += YM3812
SOUNDS += CEM3394
SOUNDS += DMADAC


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in tiny.c
#-------------------------------------------------

DRVLIBS = \
	$(EMUDRIVERS)/emudummy.o \
	$(MACHINE)/ticket.o \
	$(DRIVERS)/carpolo.o $(MACHINE)/carpolo.o $(VIDEO)/carpolo.o \
	$(DRIVERS)/circus.o $(AUDIO)/circus.o $(VIDEO)/circus.o \
	$(DRIVERS)/exidy.o $(AUDIO)/exidy.o $(VIDEO)/exidy.o \
	$(AUDIO)/exidy440.o \
	$(DRIVERS)/starfire.o $(VIDEO)/starfire.o \
	$(DRIVERS)/vertigo.o $(MACHINE)/vertigo.o $(VIDEO)/vertigo.o \
	$(DRIVERS)/victory.o $(VIDEO)/victory.o \
	$(AUDIO)/targ.o \
	$(DRIVERS)/astrocde.o $(VIDEO)/astrocde.o \
	$(DRIVERS)/gridlee.o $(AUDIO)/gridlee.o $(VIDEO)/gridlee.o \
	$(DRIVERS)/williams.o $(MACHINE)/williams.o $(AUDIO)/williams.o $(VIDEO)/williams.o \
	$(AUDIO)/gorf.o \
	$(AUDIO)/wow.o \
	$(DRIVERS)/gaelco.o $(VIDEO)/gaelco.o $(MACHINE)/gaelcrpt.o \
	$(DRIVERS)/wrally.o $(MACHINE)/wrally.o $(VIDEO)/wrally.o \
	$(DRIVERS)/looping.o \
	$(DRIVERS)/supertnk.o \



#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(DRIVERS)/30test.o:	$(LAYOUT)/30test.lh

$(DRIVERS)/8080bw.o:	$(LAYOUT)/invrvnge.lh \
			$(LAYOUT)/shuttlei.lh \
			$(LAYOUT)/cosmicm.lh

$(DRIVERS)/acefruit.o:	$(LAYOUT)/sidewndr.lh

$(DRIVERS)/ampoker2.o:	$(LAYOUT)/ampoker2.lh \
			$(LAYOUT)/sigmapkr.lh \

$(DRIVERS)/aristmk4.o:	$(LAYOUT)/aristmk4.lh \
			$(LAYOUT)/arimk4nz.lh \
			$(LAYOUT)/3bagflnz.lh \
			$(LAYOUT)/3bagflvt.lh \
			$(LAYOUT)/arcwins.lh \
			$(LAYOUT)/cgold2.lh \
			$(LAYOUT)/eforest.lh \
			$(LAYOUT)/fhunter.lh \
			$(LAYOUT)/goldenc.lh \
			$(LAYOUT)/kgbird.lh \
			$(LAYOUT)/topgear.lh \
			$(LAYOUT)/wildone.lh \
			$(LAYOUT)/gldnpkr.lh \

$(DRIVERS)/astrocde.o:	$(LAYOUT)/gorf.lh \
			$(LAYOUT)/tenpindx.lh

$(DRIVERS)/atarifb.o:	$(LAYOUT)/atarifb.lh \
			$(LAYOUT)/atarifb4.lh \
			$(LAYOUT)/abaseb.lh

$(DRIVERS)/avalnche.o:	$(LAYOUT)/avalnche.lh

$(DRIVERS)/balsente.o:	$(LAYOUT)/stocker.lh

$(DRIVERS)/bfm_sc1.o:	$(LAYOUT)/bfm_sc1.lh

$(DRIVERS)/bfm_sc2.o:	$(LAYOUT)/bfm_sc2.lh \
			$(LAYOUT)/awpdmd.lh \
			$(LAYOUT)/awpvid14.lh \
			$(LAYOUT)/awpvid16.lh \
			$(LAYOUT)/drwho.lh \
			$(LAYOUT)/gldncrwn.lh \
			$(LAYOUT)/quintoon.lh \
			$(LAYOUT)/paradice.lh \
			$(LAYOUT)/pyramid.lh \
			$(LAYOUT)/pokio.lh \
			$(LAYOUT)/slots.lh \
			$(LAYOUT)/sltblgpo.lh \
			$(LAYOUT)/sltblgtk.lh

$(DRIVERS)/blockade.o:	$(LAYOUT)/blockade.lh

$(DRIVERS)/buggychl.o:	$(LAYOUT)/buggychl.lh

$(DRIVERS)/bzone.o:	$(LAYOUT)/bzone.lh

$(DRIVERS)/cardline.o:	$(LAYOUT)/cardline.lh

$(DRIVERS)/cdi.o:	$(LAYOUT)/cdi.lh

$(DRIVERS)/changela.o:	$(LAYOUT)/changela.lh

$(DRIVERS)/chqflag.o:	$(LAYOUT)/chqflag.lh

$(DRIVERS)/cinemat.o:	$(LAYOUT)/armora.lh \
			$(LAYOUT)/solarq.lh \
			$(LAYOUT)/starcas.lh

$(DRIVERS)/cischeat.o:	$(LAYOUT)/cischeat.lh \
			$(LAYOUT)/f1gpstar.lh

$(DRIVERS)/circus.o:	$(LAYOUT)/circus.lh \
			$(LAYOUT)/crash.lh

$(DRIVERS)/copsnrob.o:	$(LAYOUT)/copsnrob.lh

$(DRIVERS)/corona.o:	$(LAYOUT)/re800.lh \
			$(LAYOUT)/luckyrlt.lh

$(DRIVERS)/darius.o:	$(LAYOUT)/darius.lh

$(DRIVERS)/destroyr.o:	$(LAYOUT)/destroyr.lh

$(DRIVERS)/dlair.o:	$(LAYOUT)/dlair.lh

$(DRIVERS)/firebeat.o:	$(LAYOUT)/firebeat.lh

$(DRIVERS)/fortecar.o:	$(LAYOUT)/fortecrd.lh

$(DRIVERS)/funworld.o:	$(LAYOUT)/jollycrd.lh \
			$(LAYOUT)/bigdeal.lh \
			$(LAYOUT)/novoplay.lh \
			$(LAYOUT)/royalcrd.lh

$(DRIVERS)/galaxi.o:	$(LAYOUT)/galaxi.lh

$(DRIVERS)/gatron.o:	$(LAYOUT)/poker41.lh \
			$(LAYOUT)/pulltabs.lh

$(DRIVERS)/goldnpkr.o:	$(LAYOUT)/goldnpkr.lh \
			$(LAYOUT)/pmpoker.lh

$(DRIVERS)/goldstar.o:	$(LAYOUT)/lucky8.lh \
			$(LAYOUT)/bingowng.lh

$(DRIVERS)/grchamp.o:	$(LAYOUT)/grchamp.lh

$(DRIVERS)/highvdeo.o:	$(LAYOUT)/fashion.lh

$(DRIVERS)/igspoker.o:	$(LAYOUT)/igspoker.lh

$(DRIVERS)/kas89.o:	$(LAYOUT)/kas89.lh

$(DRIVERS)/kingdrby.o:	$(LAYOUT)/kingdrby.lh

$(DRIVERS)/lazercmd.o:	$(LAYOUT)/lazercmd.lh

$(DRIVERS)/luckgrln.o:	$(LAYOUT)/luckgrln.lh

$(DRIVERS)/lucky74.o:	$(LAYOUT)/lucky74.lh

$(DRIVERS)/magic10.o:	$(LAYOUT)/sgsafari.lh \
			$(LAYOUT)/musicsrt.lh

$(DRIVERS)/majorpkr.o:	$(LAYOUT)/majorpkr.lh

$(DRIVERS)/maxaflex.o:	$(LAYOUT)/maxaflex.lh

$(DRIVERS)/mcr3.o:	$(LAYOUT)/turbotag.lh

$(DRIVERS)/mpoker.o:	$(LAYOUT)/mpoker.lh

$(DRIVERS)/mpu4.o:	$(LAYOUT)/mpu4.lh \
			$(LAYOUT)/connect4.lh \
			$(LAYOUT)/mpu4ext.lh \
			$(LAYOUT)/gamball.lh

$(DRIVERS)/mpu4vid.o:	$(LAYOUT)/crmaze2p.lh \
			$(LAYOUT)/crmaze4p.lh

$(DRIVERS)/mw18w.o:	$(LAYOUT)/18w.lh

$(DRIVERS)/mw8080bw.o:	$(LAYOUT)/280zzzap.lh \
			$(LAYOUT)/clowns.lh \
			$(LAYOUT)/invaders.lh \
			$(LAYOUT)/invad2ct.lh \
			$(LAYOUT)/lagunar.lh \
			$(LAYOUT)/spacwalk.lh

$(DRIVERS)/meadows.o:	$(LAYOUT)/deadeye.lh \
			$(LAYOUT)/gypsyjug.lh

$(DRIVERS)/midzeus.o:	$(LAYOUT)/crusnexo.lh

$(DRIVERS)/mil4000.o:	$(LAYOUT)/mil4000.lh

$(DRIVERS)/namcofl.o:	$(LAYOUT)/namcofl.lh

$(DRIVERS)/nbmj8688.o:	$(LAYOUT)/nbmj8688.lh

$(DRIVERS)/namcos2.o:	$(LAYOUT)/finallap.lh

$(DRIVERS)/neogeo.o:	$(LAYOUT)/neogeo.lh

$(DRIVERS)/norautp.o:	$(LAYOUT)/noraut11.lh \
			$(LAYOUT)/noraut12.lh

$(DRIVERS)/overdriv.o:	$(LAYOUT)/overdriv.lh

$(DRIVERS)/peplus.o:	$(LAYOUT)/peplus.lh \
			$(LAYOUT)/pe_schip.lh \
			$(LAYOUT)/pe_poker.lh \
			$(LAYOUT)/pe_bjack.lh \
			$(LAYOUT)/pe_keno.lh \
			$(LAYOUT)/pe_slots.lh

$(DRIVERS)/polepos.o:	$(LAYOUT)/polepos.lh \
			$(LAYOUT)/topracer.lh

$(DRIVERS)/qix.o:	$(LAYOUT)/elecyoyo.lh

$(DRIVERS)/re900.o:	$(LAYOUT)/re900.lh

$(DRIVERS)/roul.o:	$(LAYOUT)/roul.lh

$(DRIVERS)/sbrkout.o:	$(LAYOUT)/sbrkout.lh

$(DRIVERS)/sderby.o:	$(LAYOUT)/sderby.lh \
			$(LAYOUT)/spacewin.lh \
			$(LAYOUT)/pmroulet.lh

$(DRIVERS)/segaorun.o:	$(LAYOUT)/outrun.lh

$(DRIVERS)/segas32.o:	$(LAYOUT)/radr.lh

$(DRIVERS)/segasms.o:	$(LAYOUT)/sms1.lh

$(DRIVERS)/segaybd.o:	$(LAYOUT)/pdrift.lh

$(DRIVERS)/snookr10.o:	$(LAYOUT)/snookr10.lh

$(DRIVERS)/splus.o:	$(LAYOUT)/splus.lh

$(DRIVERS)/sspeedr.o:	$(LAYOUT)/sspeedr.lh

$(DRIVERS)/stactics.o:	$(LAYOUT)/stactics.lh

$(DRIVERS)/stepstag.o:	$(LAYOUT)/stepstag.lh

$(DRIVERS)/sstrangr.o:	$(LAYOUT)/sstrangr.lh

$(DRIVERS)/subsino.o:	$(LAYOUT)/victor5.lh \
			$(LAYOUT)/victor21.lh \
			$(LAYOUT)/tisub.lh \
			$(LAYOUT)/stisub.lh \
			$(LAYOUT)/crsbingo.lh \
			$(LAYOUT)/sharkpy.lh \
			$(LAYOUT)/sharkpye.lh \
			$(LAYOUT)/smoto.lh

$(DRIVERS)/superchs.o:	$(LAYOUT)/superchs.lh

$(DRIVERS)/sfbonus.o:	$(LAYOUT)/pirpok2.lh

$(DRIVERS)/taito_z.o:	$(LAYOUT)/contcirc.lh \
			$(LAYOUT)/dblaxle.lh

$(DRIVERS)/tatsumi.o:	$(LAYOUT)/roundup5.lh

$(DRIVERS)/tceptor.o:	$(LAYOUT)/tceptor2.lh

$(DRIVERS)/tehkanwc.o:	$(LAYOUT)/gridiron.lh

$(DRIVERS)/tetrisp2.o:	$(LAYOUT)/rocknms.lh

$(DRIVERS)/thayers.o:	$(LAYOUT)/dlair.lh

$(DRIVERS)/topspeed.o:	$(LAYOUT)/topspeed.lh

$(DRIVERS)/turbo.o:	$(LAYOUT)/turbo.lh \
			$(LAYOUT)/subroc3d.lh \
			$(LAYOUT)/buckrog.lh

$(DRIVERS)/tx1.o:	$(LAYOUT)/buggybjr.lh \
			$(LAYOUT)/buggyboy.lh \
			$(LAYOUT)/tx1.lh

$(DRIVERS)/umipoker.o:	$(LAYOUT)/saiyukip.lh

$(DRIVERS)/undrfire.o:	$(LAYOUT)/cbombers.lh

$(DRIVERS)/vicdual.o:	$(LAYOUT)/depthch.lh

$(DRIVERS)/videopin.o:	$(LAYOUT)/videopin.lh

$(DRIVERS)/videopkr.o:	$(LAYOUT)/videopkr.lh \
			$(LAYOUT)/blckjack.lh \
			$(LAYOUT)/videodad.lh \
			$(LAYOUT)/videocba.lh \
			$(LAYOUT)/babypkr.lh \
			$(LAYOUT)/babydad.lh

$(DRIVERS)/warpwarp.o:	$(LAYOUT)/geebee.lh \
			$(LAYOUT)/navarone.lh \
			$(LAYOUT)/sos.lh

$(DRIVERS)/wecleman.o:	$(LAYOUT)/wecleman.lh

$(DRIVERS)/zac2650.o:	$(LAYOUT)/tinv2650.lh

$(DRIVERS)/peyper.o:    $(LAYOUT)/peyper.lh

DRVLIBS += \
	$(MACHINE)/segacrpt.o \
	$(MACHINE)/segacrp2.o \

DRVLIBS += \
        $(AUDIO)/cage.o \
        $(VIDEO)/avgdvg.o \
        $(AUDIO)/taitosnd.o \
        $(AUDIO)/taito_zm.o \
        $(MACHINE)/asic65.o \

# Capcom
DRVLIBS += \
	$(DRIVERS)/1942.o $(VIDEO)/1942.o \
	$(DRIVERS)/1943.o $(VIDEO)/1943.o \
	$(DRIVERS)/bionicc.o $(VIDEO)/bionicc.o \
	$(DRIVERS)/blktiger.o $(VIDEO)/blktiger.o \
	$(DRIVERS)/cbasebal.o $(VIDEO)/cbasebal.o \
	$(DRIVERS)/commando.o $(VIDEO)/commando.o \
	$(DRIVERS)/cps1.o $(VIDEO)/cps1.o \
	$(DRIVERS)/cps2.o \
	$(DRIVERS)/cps3.o $(AUDIO)/cps3.o \
	$(DRIVERS)/egghunt.o \
	$(DRIVERS)/exedexes.o $(VIDEO)/exedexes.o \
	$(DRIVERS)/fcrash.o \
	$(DRIVERS)/gng.o $(VIDEO)/gng.o \
	$(DRIVERS)/gunsmoke.o $(VIDEO)/gunsmoke.o \
	$(DRIVERS)/higemaru.o $(VIDEO)/higemaru.o \
	$(DRIVERS)/lastduel.o $(VIDEO)/lastduel.o \
	$(DRIVERS)/lwings.o $(VIDEO)/lwings.o \
	$(DRIVERS)/mitchell.o $(VIDEO)/mitchell.o \
	$(DRIVERS)/sf.o $(VIDEO)/sf.o \
	$(DRIVERS)/sidearms.o $(VIDEO)/sidearms.o \
	$(DRIVERS)/sonson.o $(VIDEO)/sonson.o \
	$(DRIVERS)/srumbler.o $(VIDEO)/srumbler.o \
	$(DRIVERS)/tigeroad.o $(VIDEO)/tigeroad.o \
	$(DRIVERS)/vulgus.o $(VIDEO)/vulgus.o \
	$(MACHINE)/cps2crpt.o \
	$(MACHINE)/kabuki.o \

DRVLIBS += \
	$(DRIVERS)/zn.o $(MACHINE)/znsec.o \
	$(MACHINE)/psx.o

# Neogeo
DRVLIBS += \
	$(DRIVERS)/neogeo.o $(VIDEO)/neogeo.o \
	$(MACHINE)/neoboot.o \
	$(MACHINE)/neocrypt.o \
	$(MACHINE)/neoprot.o \

#midaway
DRVLIBS +=  \
	$(DRIVERS)/astrocde.o $(VIDEO)/astrocde.o \
	$(DRIVERS)/balsente.o $(MACHINE)/balsente.o $(VIDEO)/balsente.o \
	$(DRIVERS)/gridlee.o $(AUDIO)/gridlee.o $(VIDEO)/gridlee.o \
	$(DRIVERS)/mcr.o $(MACHINE)/mcr.o $(AUDIO)/mcr.o $(VIDEO)/mcr.o \
	$(DRIVERS)/mcr3.o $(VIDEO)/mcr3.o \
	$(DRIVERS)/mcr68.o $(MACHINE)/mcr68.o $(VIDEO)/mcr68.o \
	$(DRIVERS)/midqslvr.o \
	$(DRIVERS)/midtunit.o $(MACHINE)/midtunit.o $(VIDEO)/midtunit.o \
	$(DRIVERS)/midvunit.o $(VIDEO)/midvunit.o \
	$(DRIVERS)/midwunit.o $(MACHINE)/midwunit.o \
	$(DRIVERS)/midxunit.o $(MACHINE)/midxunit.o \
	$(DRIVERS)/midyunit.o $(MACHINE)/midyunit.o $(VIDEO)/midyunit.o \
	$(DRIVERS)/midzeus.o $(VIDEO)/midzeus.o $(VIDEO)/midzeus2.o \
	$(DRIVERS)/omegrace.o \
	$(DRIVERS)/seattle.o \
	$(DRIVERS)/tmaster.o \
	$(DRIVERS)/vegas.o \
	$(DRIVERS)/williams.o $(MACHINE)/williams.o $(AUDIO)/williams.o $(VIDEO)/williams.o \
	$(MACHINE)/midwayic.o \
	$(AUDIO)/dcs.o \
	$(AUDIO)/gorf.o \
	$(AUDIO)/wow.o \

# Cave


# Rare
DRVLIBS += \
	$(DRIVERS)/btoads.o $(VIDEO)/btoads.o \
	$(DRIVERS)/kinst.o \
	$(DRIVERS)/xtheball.o \
# psikyo
SOUNDS += YMF278B
DRVLIBS += \
	$(DRIVERS)/psikyo.o $(VIDEO)/psikyo.o \
	$(DRIVERS)/psikyo4.o $(VIDEO)/psikyo4.o \
	$(DRIVERS)/psikyosh.o $(VIDEO)/psikyosh.o \



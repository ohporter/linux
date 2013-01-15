/*
 * ASoC simple sound card support
 *
 * Copyright (C) 2012 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <sound/simple_card.h>

#define asoc_simple_get_card_info(p) \
	container_of(p->dai_link, struct asoc_simple_card_info, snd_link)

static int __asoc_simple_card_dai_init(struct snd_soc_dai *dai,
				       struct asoc_simple_dai *set,
				       unsigned int daifmt)
{
	int ret = 0;

	daifmt |= set->fmt;

	if (!ret && daifmt)
		ret = snd_soc_dai_set_fmt(dai, daifmt);

	if (!ret && set->sysclk)
		ret = snd_soc_dai_set_sysclk(dai, 0, set->sysclk, 0);

	return ret;
}

static int asoc_simple_card_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct asoc_simple_card_info *info = asoc_simple_get_card_info(rtd);
	struct snd_soc_dai *codec = rtd->codec_dai;
	struct snd_soc_dai *cpu = rtd->cpu_dai;
	unsigned int daifmt = info->daifmt;
	int ret;

	ret = __asoc_simple_card_dai_init(codec, &info->codec_dai, daifmt);
	if (ret < 0)
		return ret;

	ret = __asoc_simple_card_dai_init(cpu, &info->cpu_dai, daifmt);
	if (ret < 0)
		return ret;

	return 0;
}

static struct device_node*
__asoc_simple_card_parse_of(struct device_node *np,
			    const char *node_name1,
			    const char *node_name2,
			    const char **interface,
			    struct asoc_simple_dai *dai)
{
	struct device_node *node;
	char prop[128];

	/* node or name is required */
	snprintf(prop, sizeof(prop),
		 "simple-audio,%s,controller", node_name1);
	node = of_parse_phandle(np, prop, 0);
	if (node)
		of_node_put(node);

	/* get "simple-audio,xxx,yyy,name = xxx" */
	snprintf(prop, sizeof(prop),
		 "simple-audio,%s%s,name", node_name1, node_name2);
	of_property_read_string(np, prop, interface);

	if (dai) {
		/* get "simple-audio,xxx,yyy,formart = xxx" */
		snprintf(prop, sizeof(prop),
			 "simple-audio,%s%s,", node_name1, node_name2);
		dai->fmt = snd_soc_of_parse_daifmt(np, prop);

		/* get "simple-audio,xxx,yyy,system-clock-frequency = <xxx>" */
		snprintf(prop, sizeof(prop),
			 "simple-audio,%s%s,system-clock-frequency",
			 node_name1, node_name2);
		of_property_read_u32(np, prop, &dai->sysclk);
	}

	return node;
}

static void asoc_simple_card_parse_of(struct device_node *np,
				      struct asoc_simple_card_info *info,
				      struct device *dev,
				      struct device_node **of_cpu,
				      struct device_node **of_codec,
				      struct device_node **of_platform)
{
	of_property_read_string(np, "simple-audio,card-name", &info->card);
	info->name = info->card;

	*of_cpu = __asoc_simple_card_parse_of(
		np, "cpu", ",dai", &info->cpu_dai.name, &info->cpu_dai);
	*of_codec = __asoc_simple_card_parse_of(
		np, "codec", ",dai", &info->codec_dai.name, &info->codec_dai);
	*of_platform = __asoc_simple_card_parse_of(
		np, "platform", "", &info->platform, NULL);

	dev_dbg(dev, "card-name  : %s\n", info->card);
	dev_dbg(dev, "cpu info   : %s / %x / %d / %p\n",
		info->cpu_dai.name,
		info->cpu_dai.fmt,
		info->cpu_dai.sysclk,
		*of_cpu);
	dev_dbg(dev, "codec_info : %s / %x / %d / %p\n",
		info->codec_dai.name,
		info->codec_dai.fmt,
		info->codec_dai.sysclk,
		*of_codec);
	dev_dbg(dev, "platform_info : %s / %p\n",
		info->platform,
		*of_platform);
}

static int asoc_simple_card_probe(struct platform_device *pdev)
{
	struct asoc_simple_card_info *cinfo;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *of_cpu, *of_codec, *of_platform;
	struct device *dev = &pdev->dev;

	cinfo		= NULL;
	of_cpu		= NULL;
	of_codec	= NULL;
	of_platform	= NULL;
	if (np && of_device_is_available(np)) {
		cinfo = devm_kzalloc(dev, sizeof(*cinfo), GFP_KERNEL);
		if (cinfo)
			asoc_simple_card_parse_of(np, cinfo, dev,
						  &of_cpu,
						  &of_codec,
						  &of_platform);
	} else {
		cinfo = pdev->dev.platform_data;
	}

	if (!cinfo) {
		dev_err(dev, "no info for asoc-simple-card\n");
		return -EINVAL;
	}

	if (!cinfo->name	||
	    !cinfo->card	||
	    !cinfo->codec_dai.name	||
	    !(cinfo->codec		|| of_codec)	||
	    !(cinfo->platform		|| of_platform)	||
	    !(cinfo->cpu_dai.name	|| of_cpu)) {
		dev_err(dev, "insufficient asoc_simple_card_info settings\n");
		return -EINVAL;
	}

	/*
	 * init snd_soc_dai_link
	 */
	cinfo->snd_link.name		= cinfo->name;
	cinfo->snd_link.stream_name	= cinfo->name;
	cinfo->snd_link.cpu_dai_name	= cinfo->cpu_dai.name;
	cinfo->snd_link.platform_name	= cinfo->platform;
	cinfo->snd_link.codec_name	= cinfo->codec;
	cinfo->snd_link.codec_dai_name	= cinfo->codec_dai.name;
	cinfo->snd_link.cpu_of_node	= of_cpu;
	cinfo->snd_link.codec_of_node	= of_codec;
	cinfo->snd_link.platform_of_node = of_platform;
	cinfo->snd_link.init		= asoc_simple_card_dai_init;

	/*
	 * init snd_soc_card
	 */
	cinfo->snd_card.name		= cinfo->card;
	cinfo->snd_card.owner		= THIS_MODULE;
	cinfo->snd_card.dai_link	= &cinfo->snd_link;
	cinfo->snd_card.num_links	= 1;
	cinfo->snd_card.dev		= &pdev->dev;

	return snd_soc_register_card(&cinfo->snd_card);
}

static int asoc_simple_card_remove(struct platform_device *pdev)
{
	struct asoc_simple_card_info *cinfo = pdev->dev.platform_data;

	return snd_soc_unregister_card(&cinfo->snd_card);
}

static const struct of_device_id asoc_simple_of_match[] = {
	{ .compatible = "simple-audio", },
	{},
};

static struct platform_driver asoc_simple_card = {
	.driver = {
		.name	= "asoc-simple-card",
		.of_match_table = asoc_simple_of_match,
	},
	.probe		= asoc_simple_card_probe,
	.remove		= asoc_simple_card_remove,
};

module_platform_driver(asoc_simple_card);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC Simple Sound Card");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");
